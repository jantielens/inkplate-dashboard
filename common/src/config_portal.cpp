#include "config_portal.h"
#include "config_portal_css.h"
#include "version.h"
#include "config.h"
#include <src/logo_bitmap.h>
#include "logger.h"
#include "github_ota.h"

ConfigPortal::ConfigPortal(ConfigManager* configManager, WiFiManager* wifiManager, DisplayManager* displayManager)
    : _configManager(configManager), _wifiManager(wifiManager), _displayManager(displayManager),
      _server(nullptr), _configReceived(false), _port(80), _mode(CONFIG_MODE) {
}

ConfigPortal::~ConfigPortal() {
    stop();
}

bool ConfigPortal::begin(PortalMode mode) {
    if (_server != nullptr) {
        LogBox::begin("Config Portal");
        LogBox::line("Config portal already running");
        LogBox::end();
        return true;
    }
    
    _mode = mode;
    _server = new WebServer(_port);
    
    // Set up routes
    _server->on("/", [this]() { this->handleRoot(); });
    _server->on("/submit", HTTP_POST, [this]() { this->handleSubmit(); });
    _server->on("/factory-reset", HTTP_POST, [this]() { this->handleFactoryReset(); });
    _server->on("/reboot", HTTP_POST, [this]() { this->handleReboot(); });
    #ifndef DISPLAY_MODE_INKPLATE2
    // VCOM routes only available on boards with TPS65186 PMIC (not Inkplate 2)
    _server->on("/vcom", HTTP_GET, [this]() { this->handleVcom(); });
    _server->on("/vcom", HTTP_POST, [this]() { this->handleVcomSubmit(); });
    #endif
    // OTA routes - only available in CONFIG_MODE
    if (_mode == CONFIG_MODE) {
        _server->on("/ota", HTTP_GET, [this]() { this->handleOTA(); });
        _server->on("/ota/check", HTTP_GET, [this]() { this->handleOTACheck(); });
        _server->on("/ota/install", HTTP_POST, [this]() { this->handleOTAInstall(); });
        _server->on("/ota/status", HTTP_GET, [this]() { this->handleOTAStatus(); });
        _server->on("/ota/progress", HTTP_GET, [this]() { this->handleOTAProgress(); });
        _server->on("/ota", HTTP_POST, 
            [this]() { this->handleOTAUpload(); },
            [this]() { 
                // Handle upload data
                HTTPUpload& upload = _server->upload();
                if (upload.status == UPLOAD_FILE_START) {
                    LogBox::begin("OTA Update");
                    LogBox::line("Starting OTA update: " + upload.filename);
                    LogBox::end();
                    
                    // Show visual feedback on screen
                    if (_displayManager != nullptr) {
                        _displayManager->clear();
                        // Draw logo at top area for visual branding
                        int screenWidth = _displayManager->getWidth();
                        int minLogoX = MARGIN;
                        int maxLogoX = screenWidth - LOGO_WIDTH - MARGIN;
                        int logoX;
                        if (maxLogoX <= minLogoX) {
                            logoX = minLogoX;
                        } else {
                            logoX = minLogoX + (maxLogoX - minLogoX) / 2;
                        }
                        int logoY = MARGIN;
#ifndef DISPLAY_MODE_INKPLATE2
                        _displayManager->drawBitmap(logo_bitmap, logoX, logoY, LOGO_WIDTH, LOGO_HEIGHT);
#endif
                        int y = logoY + LOGO_HEIGHT + MARGIN;
                        _displayManager->showMessage("Firmware Update", MARGIN, y, FONT_HEADING1);
                        y += _displayManager->getFontHeight(FONT_HEADING1) + LINE_SPACING * 2;
                        _displayManager->showMessage("Installing firmware...", MARGIN, y, FONT_NORMAL);
                        y += _displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
                        _displayManager->showMessage("Device will reboot when complete.", MARGIN, y, FONT_NORMAL);
                        y += _displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING * 2;
                        _displayManager->showMessage("Do not power off!", MARGIN, y, FONT_NORMAL);
                        _displayManager->refresh();
                    }
                    
                    // Disable watchdog timer to prevent reboot during update
                    disableCore0WDT();
                    
                    // Get the size of the next available OTA partition
                    size_t updateSize = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
                    
                    // Begin update with calculated size
                    if (!Update.begin(updateSize, U_FLASH)) {
                        Update.printError(Serial);
                        enableCore0WDT();  // Re-enable on failure
                    }
                } else if (upload.status == UPLOAD_FILE_WRITE) {
                    // Write data to flash
                    size_t written = Update.write(upload.buf, upload.currentSize);
                    if (written != upload.currentSize) {
                        Update.printError(Serial);
                    }
                } else if (upload.status == UPLOAD_FILE_END) {
                    if (Update.end(true)) {
                        LogBox::begin("OTA Success");
                        LogBox::linef("Update successful: %u bytes", upload.totalSize);
                        LogBox::end();
                        // Watchdog will be reset on reboot, no need to re-enable
                    } else {
                        Update.printError(Serial);
                        enableCore0WDT();  // Re-enable on failure
                    }
                } else if (upload.status == UPLOAD_FILE_ABORTED) {
                    Update.end();
                    enableCore0WDT();  // Re-enable on abort
                }
            }
        );
    }
    
    _server->onNotFound([this]() { this->handleNotFound(); });
    
    _server->begin();
    LogBox::begin("Config Portal Started");
    LogBox::linef("Port: %d", _port);
    LogBox::line("Open http://" + _wifiManager->getAPIPAddress());
    LogBox::end();
    
    return true;
}

void ConfigPortal::stop() {
    if (_server != nullptr) {
        _server->stop();
        delete _server;
        _server = nullptr;
        LogBox::begin("Config Portal");
        LogBox::line("Configuration portal stopped");
        LogBox::end();
    }
}

void ConfigPortal::handleClient() {
    if (_server != nullptr) {
        _server->handleClient();
    }
}

bool ConfigPortal::isConfigReceived() {
    return _configReceived;
}

int ConfigPortal::getPort() {
    return _port;
}

void ConfigPortal::handleRoot() {
    LogBox::begin("Web Request");
    LogBox::line("Serving configuration page");
    LogBox::end();
    _server->send(200, "text/html", generateConfigPage());
}

void ConfigPortal::handleSubmit() {
    LogBox::begin("Web Request");
    LogBox::line("Configuration form submitted");
    LogBox::end();
    
    // Get form data
    String ssid = _server->arg("ssid");
    String password = _server->arg("password");
    String imageUrl = _server->arg("imageurl");
    String refreshRateStr = _server->arg("refresh");
    String mqttBroker = _server->arg("mqttbroker");
    String mqttUser = _server->arg("mqttuser");
    String mqttPass = _server->arg("mqttpass");
    String timezoneStr = _server->arg("timezone");
    String rotationStr = _server->arg("rotation");
    bool debugMode = _server->hasArg("debugmode") && _server->arg("debugmode") == "on";
    bool useCRC32Check = _server->hasArg("crc32check") && _server->arg("crc32check") == "on";
    
    // Parse timezone offset
    int timezoneOffset = timezoneStr.toInt();
    if (timezoneOffset < -12 || timezoneOffset > 14) {
        timezoneOffset = 0;  // Default to UTC on invalid input
    }
    
    // Parse screen rotation
    uint8_t screenRotation = rotationStr.toInt();
    if (screenRotation > 3) {
        screenRotation = 0;  // Default to 0° on invalid input
    }
    
    // Parse hourly schedule bitmask (24 checkboxes)
    uint8_t updateHours[3] = {0, 0, 0};
    for (int hour = 0; hour < 24; hour++) {
        String hourKey = "hour_" + String(hour);
        if (_server->hasArg(hourKey) && _server->arg(hourKey) == "on") {
            uint8_t byteIndex = hour / 8;
            uint8_t bitPosition = hour % 8;
            updateHours[byteIndex] |= (1 << bitPosition);
        }
    }
    
    // Validate input
    if (ssid.length() == 0) {
        _server->send(400, "text/html", generateErrorPage("WiFi SSID is required"));
        return;
    }
    
    // In CONFIG_MODE, image URL is required; in BOOT_MODE it's optional
    if (_mode == CONFIG_MODE && imageUrl.length() == 0) {
        _server->send(400, "text/html", generateErrorPage("Image URL is required"));
        return;
    }
    
    int refreshRate = refreshRateStr.toInt();
    if (refreshRate < 1) {
        refreshRate = DEFAULT_REFRESH_RATE;
    }
    
    // In BOOT_MODE, only save WiFi credentials
    if (_mode == BOOT_MODE) {
        _configManager->setWiFiCredentials(ssid, password);
        LogBox::begin("Config Saved");
        LogBox::line("WiFi credentials saved (boot mode)");
        LogBox::end();
        _configReceived = true;
        _server->send(200, "text/html", generateSuccessPage());
        return;
    }
    
    // In CONFIG_MODE, save all configuration
    DashboardConfig config;
    config.wifiSSID = ssid;
    config.imageURL = imageUrl;
    config.refreshRate = refreshRate;
    config.mqttBroker = mqttBroker;
    config.mqttUsername = mqttUser;
    config.debugMode = debugMode;
    config.useCRC32Check = useCRC32Check;
    config.updateHours[0] = updateHours[0];
    config.updateHours[1] = updateHours[1];
    config.updateHours[2] = updateHours[2];
    config.timezoneOffset = timezoneOffset;
    config.screenRotation = screenRotation;
    
    // Handle WiFi password - if empty and device is configured, keep existing password
    if (password.length() == 0 && _configManager->isConfigured()) {
        config.wifiPassword = _configManager->getWiFiPassword();
        LogBox::begin("Config Update");
        LogBox::line("Keeping existing WiFi password");
        LogBox::end();
    } else {
        config.wifiPassword = password;
    }
    
    // Handle MQTT password - if empty and device is configured with MQTT, keep existing password
    if (mqttPass.length() == 0 && _configManager->isConfigured() && _configManager->getMQTTPassword().length() > 0) {
        config.mqttPassword = _configManager->getMQTTPassword();
        LogBox::begin("Config Update");
        LogBox::line("Keeping existing MQTT password");
        LogBox::end();
    } else {
        config.mqttPassword = mqttPass;
    }
    
    // Save configuration
    if (_configManager->saveConfig(config)) {
        LogBox::begin("Config Saved");
        LogBox::line("Configuration saved successfully");
        LogBox::end();
        _configReceived = true;
        _server->send(200, "text/html", generateSuccessPage());
    } else {
        LogBox::begin("Config Error");
        LogBox::line("Failed to save configuration");
        LogBox::end();
        _server->send(500, "text/html", generateErrorPage("Failed to save configuration"));
    }
}

void ConfigPortal::handleFactoryReset() {
    LogBox::begin("Factory Reset");
    LogBox::line("Factory reset requested");
    LogBox::end();
    
    // Clear all configuration
    _configManager->clearConfig();
    
    // Send success page
    _server->send(200, "text/html", generateFactoryResetPage());
    
    // Device will reboot after the response is sent
    LogBox::begin("Factory Reset");
    LogBox::line("Factory reset completed");
    LogBox::line("Device will reboot in 2 seconds");
    LogBox::end();
    delay(2000);
    ESP.restart();
}

void ConfigPortal::handleReboot() {
    LogBox::begin("Reboot");
    LogBox::line("Device reboot requested");
    LogBox::end();
    
    // Send reboot page
    _server->send(200, "text/html", generateRebootPage());
    
    // Device will reboot after the response is sent
    LogBox::begin("Reboot");
    LogBox::line("Device will reboot in 2 seconds");
    LogBox::end();
    delay(2000);
    ESP.restart();
}

void ConfigPortal::handleNotFound() {
    _server->send(404, "text/html", generateErrorPage("Page not found"));
}

String ConfigPortal::getCSS() {
    return String(CONFIG_PORTAL_CSS);
}

String ConfigPortal::generateConfigPage() {
    // Load current configuration if available
    DashboardConfig currentConfig;
    bool hasConfig = _configManager->isConfigured() && _configManager->loadConfig(currentConfig);
    
    // Also check for partial config (WiFi but no Image URL) - for Step 2
    bool hasPartialConfig = false;
    if (!hasConfig && _configManager->hasWiFiConfig()) {
        currentConfig.wifiSSID = _configManager->getWiFiSSID();
        currentConfig.wifiPassword = _configManager->getWiFiPassword();
        hasPartialConfig = true;
    }
    
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Inkplate Dashboard Setup</title>";
    html += getCSS();
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<h1>📊 Inkplate Dashboard</h1>";
    
    // Different subtitle based on mode
    if (_mode == BOOT_MODE) {
        html += "<p class='subtitle'>Step 1: Connect to WiFi</p>";
    } else if (hasConfig) {
        html += "<p class='subtitle'>Update your dashboard configuration</p>";
    } else {
        html += "<p class='subtitle'>Step 2: Configure your dashboard</p>";
    }
    
    html += "<div class='device-info'>";
    html += "<strong>Device:</strong> " + _wifiManager->getAPName() + "<br>";
    
    // Show appropriate IP address
    if (_wifiManager->isConnected()) {
        html += "<strong>IP:</strong> " + _wifiManager->getLocalIP();
    } else {
        html += "<strong>IP:</strong> " + _wifiManager->getAPIPAddress();
    }
    html += "</div>";
    
    html += "<form action='/submit' method='POST'>";
    
    // WiFi SSID - always shown
    html += "<div class='form-group'>";
    html += "<label for='ssid'>WiFi Network Name (SSID) *</label>";
    if (hasConfig || hasPartialConfig) {
        html += "<input type='text' id='ssid' name='ssid' required placeholder='Enter your WiFi network name' value='" + currentConfig.wifiSSID + "'>";
    } else {
        html += "<input type='text' id='ssid' name='ssid' required placeholder='Enter your WiFi network name'>";
    }
    html += "</div>";
    
    // WiFi Password - always shown
    html += "<div class='form-group'>";
    html += "<label for='password'>WiFi Password</label>";
    if ((hasConfig || hasPartialConfig) && currentConfig.wifiPassword.length() > 0) {
        html += "<input type='password' id='password' name='password' placeholder='Enter WiFi password (leave empty if none)' value='" + currentConfig.wifiPassword + "'>";
        html += "<div class='help-text'>Password is set. Leave empty to keep current password.</div>";
    } else {
        html += "<input type='password' id='password' name='password' placeholder='Enter WiFi password (leave empty if none)'>";
    }
    html += "</div>";
    
    // Image URL - only shown in CONFIG_MODE
    if (_mode == CONFIG_MODE) {
        html += "<div class='form-group'>";
        html += "<label for='imageurl'>Image URL *</label>";
        if (hasConfig) {
            html += "<input type='text' id='imageurl' name='imageurl' required placeholder='https://example.com/image.png' value='" + currentConfig.imageURL + "'>";
        } else {
            html += "<input type='text' id='imageurl' name='imageurl' required placeholder='https://example.com/image.png'>";
        }
        html += "<div class='help-text'>Must be a PNG image matching your screen resolution</div>";
        html += "</div>";
        
        // Timezone Offset
        html += "<div class='form-group'>";
        html += "<label for='timezone'>Timezone Offset (UTC)</label>";
        if (hasConfig) {
            html += "<input type='number' id='timezone' name='timezone' min='-12' max='14' value='" + String(currentConfig.timezoneOffset) + "' placeholder='0'>";
        } else {
            html += "<input type='number' id='timezone' name='timezone' min='-12' max='14' value='0' placeholder='0'>";
        }
        html += "<div class='help-text'>Enter your timezone offset (range: -12 to +14). Keep in mind that Daylight Saving Time may apply in your region - you'll need to update this offset when DST changes.</div>";
        html += "</div>";
        
        // Screen Rotation
        html += "<div class='form-group'>";
        html += "<label for='rotation'>Screen Rotation</label>";
        html += "<select id='rotation' name='rotation'>";
        uint8_t currentRotation = hasConfig ? currentConfig.screenRotation : 0;
        html += "<option value='0'" + String(currentRotation == 0 ? " selected" : "") + ">0° (Landscape)</option>";
        html += "<option value='1'" + String(currentRotation == 1 ? " selected" : "") + ">90° (Portrait)</option>";
        html += "<option value='2'" + String(currentRotation == 2 ? " selected" : "") + ">180° (Inverted Landscape)</option>";
        html += "<option value='3'" + String(currentRotation == 3 ? " selected" : "") + ">270° (Portrait Inverted)</option>";
        html += "</select>";
        html += "<div class='help-text'>Select the orientation of your display. Important: Your images must be oriented to match this setting (e.g., for 90° portrait, provide a portrait-oriented image).</div>";
        html += "</div>";
        
        // Debug mode toggle
        html += "<div class='form-group'>";
        html += "<label for='debugmode' style='display: flex; align-items: center; gap: 10px;'>";
        html += "<input type='checkbox' id='debugmode' name='debugmode'";
        if (hasConfig && currentConfig.debugMode) {
            html += " checked";
        }
        html += "> Enable on-screen debug messages";
        html += "</label>";
        html += "<div class='help-text'>When disabled, only the final image or error appears on the display.</div>";
        html += "</div>";
        
        // MQTT Configuration - optional section
        html += "<div class='form-group' style='margin-top: 30px; padding-top: 20px; border-top: 2px solid #e0e0e0;'>";
        html += "<label style='font-size: 18px; margin-bottom: 5px;'>📡 MQTT / Home Assistant (Optional)</label>";
        html += "<div class='help-text' style='margin-bottom: 15px;'>Configure MQTT to send battery voltage to Home Assistant</div>";
        html += "</div>";
        
        // MQTT Broker URL
        html += "<div class='form-group'>";
        html += "<label for='mqttbroker'>MQTT Broker URL</label>";
        if (hasConfig) {
            html += "<input type='text' id='mqttbroker' name='mqttbroker' placeholder='mqtt://broker.example.com:1883' value='" + currentConfig.mqttBroker + "'>";
        } else {
            html += "<input type='text' id='mqttbroker' name='mqttbroker' placeholder='mqtt://broker.example.com:1883'>";
        }
        html += "<div class='help-text'>Leave empty to disable MQTT reporting</div>";
        html += "</div>";
        
        // MQTT Username
        html += "<div class='form-group'>";
        html += "<label for='mqttuser'>MQTT Username (optional)</label>";
        if (hasConfig) {
            html += "<input type='text' id='mqttuser' name='mqttuser' placeholder='username' value='" + currentConfig.mqttUsername + "'>";
        } else {
            html += "<input type='text' id='mqttuser' name='mqttuser' placeholder='username'>";
        }
        html += "</div>";
        
        // MQTT Password
        html += "<div class='form-group'>";
        html += "<label for='mqttpass'>MQTT Password (optional)</label>";
        if (hasConfig && currentConfig.mqttPassword.length() > 0) {
            html += "<input type='password' id='mqttpass' name='mqttpass' placeholder='password' value='" + currentConfig.mqttPassword + "'>";
            html += "<div class='help-text'>Password is set. Leave empty to keep current password.</div>";
        } else {
            html += "<input type='password' id='mqttpass' name='mqttpass' placeholder='password'>";
        }
        html += "</div>";
        
        // Power & Update Schedule Section
        html += "<div class='form-group' style='margin-top: 30px; padding-top: 20px; border-top: 2px solid #e0e0e0;'>";
        html += "<label style='font-size: 18px; margin-bottom: 5px;'>⚡ Power & Update Schedule</label>";
        html += "<div class='help-text' style='margin-bottom: 15px;'>Configure update frequency and schedule to optimize battery life</div>";
        html += "</div>";
        
        // Refresh Rate
        html += "<div class='form-group'>";
        html += "<label for='refresh'>Refresh Rate (minutes)</label>";
        if (hasConfig) {
            html += "<input type='number' id='refresh' name='refresh' min='1' value='" + String(currentConfig.refreshRate) + "' placeholder='5'>";
        } else {
            html += "<input type='number' id='refresh' name='refresh' min='1' value='5' placeholder='5'>";
        }
        html += "<div class='help-text'>How often to update the image (default: 5 minutes)</div>";
        html += "</div>";
        
        // CRC32 change detection toggle
        html += "<div class='form-group'>";
        html += "<label for='crc32check' style='display: flex; align-items: center; gap: 10px;'>";
        html += "<input type='checkbox' id='crc32check' name='crc32check'";
        if (hasConfig && currentConfig.useCRC32Check) {
            html += " checked";
        }
        html += "> Enable CRC32-based change detection";
        html += "</label>";
        html += "<div class='help-text'>Skips image download & refresh when unchanged. Requires compatible web server that generates .crc32 checksum files (naming: image.png.crc32). Significantly extends battery life.</div>";
        html += "</div>";
        
        // Hourly Schedule - Update Hours
        html += "<div class='form-group' style='margin-top: 20px;'>";
        html += "<label style='font-size: 16px; margin-bottom: 5px;'>📅 Update Hours</label>";
        html += "<div class='help-text' style='margin-bottom: 15px;'>Select which hours the device should perform updates. Unchecked hours will be skipped to save battery.</div>";
        
        // 24 checkboxes in a 4x6 grid (4 columns for better UI fit)
        html += "<div style='display: grid; grid-template-columns: repeat(4, 1fr); gap: 10px; margin-bottom: 20px;'>";
        for (int hour = 0; hour < 24; hour++) {
            // Check if hour is enabled: use current config if available, otherwise default to enabled
            bool isEnabled = hasConfig ? 
                ((currentConfig.updateHours[hour / 8] >> (hour % 8)) & 1) : 
                true;  // Default all enabled if no config
            
            int nextHour = (hour + 1) % 24;
            
            html += "<label style='display: flex; align-items: center; gap: 8px; padding: 8px; background: #f5f5f5; border-radius: 4px; cursor: pointer;'>";
            html += "<input type='checkbox' id='hour_" + String(hour) + "' name='hour_" + String(hour) + "' class='hour-checkbox'";
            if (isEnabled) {
                html += " checked";
            }
            html += "> <div style='line-height: 1.2;'><div>" + String(hour < 10 ? "0" : "") + String(hour) + ":00</div>";
            html += "<div style='font-size: 11px; color: #999; margin-top: 1px;'>to " + String(nextHour < 10 ? "0" : "") + String(nextHour) + ":00</div></div>";
            html += "</label>";
        }
        html += "</div>";
        html += "</div>";
        
        // Battery Life Estimator - placed after all power-impacting settings
        html += CONFIG_PORTAL_BATTERY_ESTIMATOR_HTML;
    }
    
    // Submit button - text varies by mode
    if (_mode == BOOT_MODE) {
        html += "<button type='submit'>➡️ Next: Configure Dashboard</button>";
    } else if (hasConfig) {
        html += "<button type='submit'>🔄 Update Configuration</button>";
    } else {
        html += "<button type='submit'>💾 Save Configuration</button>";
    }
    html += "</form>";
    
    // OTA Update button - only shown in CONFIG_MODE
    if (_mode == CONFIG_MODE) {
        html += "<div style='margin-top: 20px;'>";
        html += "<a href='/ota' style='display: block; text-decoration: none;'>";
        html += "<button type='button' class='btn-secondary'>⬆️ Firmware Update</button>";
        html += "</a>";
        html += "</div>";
        
        // Reboot button - only shown in CONFIG_MODE
        html += "<div style='margin-top: 10px;'>";
        html += "<form method='POST' action='/reboot' style='display: block;'>";
        html += "<button type='submit' class='btn-secondary' style='width: 100%;'>🔄 Reboot Device</button>";
        html += "</form>";
        html += "</div>";
    }
    
    // Factory Reset & VCOM Section - only show in CONFIG_MODE
    if (_mode == CONFIG_MODE) {
        html += "<div class='factory-reset-section'>";
        html += "<div class='danger-zone'>";
        html += "<h2>⚠️ Danger Zone</h2>";
        html += "<p>Factory reset will erase all settings including WiFi credentials and configuration. The device will reboot and start fresh.</p>";
        html += "<button class='btn-danger' onclick='showResetModal()'>🗑️ Factory Reset</button>";
        #ifndef DISPLAY_MODE_INKPLATE2
        // VCOM management only available on boards with TPS65186 PMIC (not Inkplate 2)
        html += "<p style='margin-top:20px;'>VCOM management allows you to view and adjust the display panel's VCOM voltage. This is an advanced feature for correcting image artifacts or ghosting. Use with caution.</p>";
        html += "<button class='btn-danger' style='width:100%; margin-top:10px;' onclick=\"window.location.href='/vcom'\">⚠️ VCOM Management</button>";
        #endif
        html += "</div>";
        html += "</div>";
    }
    
    html += "</div>";
    
    // Modal dialog for factory reset confirmation (only in CONFIG_MODE)
    if (_mode == CONFIG_MODE && hasConfig) {
        html += CONFIG_PORTAL_RESET_MODAL_HTML;
        
        // JavaScript for modal
        html += "<script>";
        html += "function showResetModal() { document.getElementById('resetModal').style.display = 'block'; }";
        html += "function hideResetModal() { document.getElementById('resetModal').style.display = 'none'; }";
        html += "window.onclick = function(event) { var modal = document.getElementById('resetModal'); if (event.target == modal) { modal.style.display = 'none'; } }";
        html += "</script>";
    }
    
    // Footer with version
    String footer = CONFIG_PORTAL_FOOTER_TEMPLATE;
    footer.replace("%VERSION%", String(FIRMWARE_VERSION));
    html += footer;
    
    // Battery Life Estimator JavaScript - only in CONFIG_MODE
    if (_mode == CONFIG_MODE) {
        html += "<script>";
        
        // Power consumption constants - well commented for future refinement
        html += "const POWER_CONSTANTS = {";
        html += "  ACTIVE_MA: 100,";  // WiFi active (downloading, connecting) in milliamps
        html += "  DISPLAY_MA: 50,";  // E-paper display update power consumption in mA
        html += "  SLEEP_MA: 0.02,";  // Deep sleep mode: ~20 microamps = 0.02 milliamps
        html += "  IMAGE_UPDATE_SEC: 7,";  // Time for full image update: WiFi + download + display
        html += "  CRC32_CHECK_SEC: 1";  // Time for quick CRC32 check: only checksum download
        html += "};";
        
        // Main calculation function
        html += "function calculateBatteryLife() {";
        html += "  const refreshRate = parseInt(document.getElementById('refresh').value) || 5;";
        html += "  const useCRC32 = document.getElementById('crc32check').checked;";
        html += "  const batteryCapacity = parseInt(document.getElementById('battery-capacity').value) || 1200;";
        html += "  const dailyChanges = parseInt(document.getElementById('daily-changes').value) || 5;";
        html += "  ";
        html += "  let activeHours = 0;";
        html += "  for (let hour = 0; hour < 24; hour++) {";
        html += "    const checkbox = document.getElementById('hour_' + hour);";
        html += "    if (checkbox && checkbox.checked) activeHours++;";
        html += "  }";
        html += "  if (activeHours === 0) activeHours = 24;";
        html += "  ";
        html += "  const wakeupsPerDay = activeHours * (60 / refreshRate);";
        html += "  let dailyPower = 0;";
        html += "  let activeTimeMinutes = 0;";
        html += "  ";
        html += "  if (useCRC32) {";
        html += "    const fullUpdates = Math.min(dailyChanges, wakeupsPerDay);";  // Can't have more updates than wake-ups
        html += "    const crc32Checks = wakeupsPerDay - fullUpdates;";
        html += "    const activeSecondsPerUpdate = 5;";
        html += "    const displaySecondsPerUpdate = 2;";
        html += "    const powerPerFullUpdate = (activeSecondsPerUpdate * POWER_CONSTANTS.ACTIVE_MA / 3600) + (displaySecondsPerUpdate * POWER_CONSTANTS.DISPLAY_MA / 3600);";
        html += "    dailyPower += fullUpdates * powerPerFullUpdate;";
        html += "    const powerPerCRC32Check = (POWER_CONSTANTS.CRC32_CHECK_SEC * POWER_CONSTANTS.ACTIVE_MA / 3600);";
        html += "    dailyPower += crc32Checks * powerPerCRC32Check;";
        html += "    activeTimeMinutes = (fullUpdates * POWER_CONSTANTS.IMAGE_UPDATE_SEC + crc32Checks * POWER_CONSTANTS.CRC32_CHECK_SEC) / 60;";
        html += "  } else {";
        html += "    const activeSecondsPerUpdate = 5;";
        html += "    const displaySecondsPerUpdate = 2;";
        html += "    const powerPerFullUpdate = (activeSecondsPerUpdate * POWER_CONSTANTS.ACTIVE_MA / 3600) + (displaySecondsPerUpdate * POWER_CONSTANTS.DISPLAY_MA / 3600);";
        html += "    dailyPower += wakeupsPerDay * powerPerFullUpdate;";
        html += "    activeTimeMinutes = (wakeupsPerDay * POWER_CONSTANTS.IMAGE_UPDATE_SEC) / 60;";
        html += "  }";
        html += "  ";
        html += "  const sleepHours = 24 - (activeTimeMinutes / 60);";
        html += "  dailyPower += sleepHours * POWER_CONSTANTS.SLEEP_MA;";
        html += "  ";
        html += "  const batteryLifeDays = Math.round(batteryCapacity / dailyPower);";
        html += "  const batteryLifeMonths = (batteryLifeDays / 30).toFixed(1);";
        html += "  ";
        html += "  let status = 'poor', statusText = 'SHORT';";
        html += "  if (batteryLifeDays >= 180) { status = 'excellent'; statusText = 'EXCELLENT'; }";
        html += "  else if (batteryLifeDays >= 90) { status = 'good'; statusText = 'GOOD'; }";
        html += "  else if (batteryLifeDays >= 45) { status = 'moderate'; statusText = 'MODERATE'; }";
        html += "  ";
        html += "  document.getElementById('battery-days').textContent = batteryLifeDays + ' days';";
        html += "  document.getElementById('battery-months').textContent = 'Approximately ' + batteryLifeMonths + ' months';";
        html += "  document.getElementById('daily-power').textContent = dailyPower.toFixed(1) + ' mAh';";
        html += "  document.getElementById('wakeups').textContent = wakeupsPerDay;";
        html += "  document.getElementById('active-time').textContent = activeTimeMinutes.toFixed(1) + ' min/day';";
        html += "  document.getElementById('sleep-time').textContent = sleepHours.toFixed(1) + ' hrs/day';";
        html += "  ";
        html += "  document.getElementById('status-badge').textContent = statusText;";
        html += "  document.getElementById('status-badge').className = 'status-badge status-' + status;";
        html += "  document.getElementById('battery-result').className = 'battery-result ' + status;";
        html += "  ";
        html += "  const progressBar = document.getElementById('progress-bar');";
        html += "  const progressPercent = Math.min(100, (batteryLifeDays / 300) * 100);";
        html += "  progressBar.style.width = progressPercent + '%';";
        html += "  progressBar.className = 'battery-progress-bar ' + status;";
        html += "  ";
        html += "  const tipsDiv = document.getElementById('battery-tips');";
        html += "  const tipText = document.getElementById('tip-text');";
        html += "  if (!useCRC32) {";
        html += "    tipsDiv.style.display = 'block';";
        html += "    tipText.textContent = 'Enable CRC32 change detection to extend battery life by 5-8×!';";
        html += "  } else if (refreshRate <= 2) {";
        html += "    tipsDiv.style.display = 'block';";
        html += "    tipText.textContent = 'Consider increasing refresh rate to 5-10 minutes to extend battery life.';";
        html += "  } else if (batteryLifeDays < 60) {";
        html += "    tipsDiv.style.display = 'block';";
        html += "    tipText.textContent = 'Consider using a larger battery or reducing refresh frequency.';";
        html += "  } else {";
        html += "    tipsDiv.style.display = 'none';";
        html += "  }";
        html += "}";
        
        // Event listeners setup
        html += "document.addEventListener('DOMContentLoaded', function() {";
        html += "  document.getElementById('refresh').addEventListener('input', calculateBatteryLife);";
        html += "  document.getElementById('crc32check').addEventListener('change', calculateBatteryLife);";
        html += "  document.getElementById('battery-capacity').addEventListener('change', calculateBatteryLife);";
        html += "  document.getElementById('daily-changes').addEventListener('input', calculateBatteryLife);";
        html += "  for (let hour = 0; hour < 24; hour++) {";
        html += "    const checkbox = document.getElementById('hour_' + hour);";
        html += "    if (checkbox) checkbox.addEventListener('change', calculateBatteryLife);";
        html += "  }";
        html += "  calculateBatteryLife();";
        html += "});";
        html += "</script>";
    }
    
    html += "</body></html>";
    
    return html;
}

String ConfigPortal::generateSuccessPage() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Configuration Saved</title>";
    html += getCSS();
    html += "<meta http-equiv='refresh' content='5;url=/'>";
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<div class='success'>";
    html += "<h1>✅ Success!</h1>";
    html += "<p style='margin-top: 15px;'>Configuration saved successfully.</p>";
    html += "<p style='margin-top: 10px;'>The device will restart and connect to your WiFi network.</p>";
    html += "<p style='margin-top: 15px; font-size: 14px;'>This page will redirect in 5 seconds...</p>";
    html += "</div>";
    
    String footer = CONFIG_PORTAL_FOOTER_TEMPLATE;
    footer.replace("%VERSION%", String(FIRMWARE_VERSION));
    html += footer;
    
    html += "</div>";
    html += "</body></html>";
    
    return html;
}

String ConfigPortal::generateErrorPage(const String& error) {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Error</title>";
    html += getCSS();
    html += "<meta http-equiv='refresh' content='3;url=/'>";
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<div class='error'>";
    html += "<h1>❌ Error</h1>";
    html += "<p style='margin-top: 15px;'>" + error + "</p>";
    html += "<p style='margin-top: 15px; font-size: 14px;'>Redirecting back in 3 seconds...</p>";
    html += "</div>";
    
    String footer = CONFIG_PORTAL_FOOTER_TEMPLATE;
    footer.replace("%VERSION%", String(FIRMWARE_VERSION));
    html += footer;
    
    html += "</div>";
    html += "</body></html>";
    
    return html;
}

String ConfigPortal::generateFactoryResetPage() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Factory Reset</title>";
    html += getCSS();
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<div class='success'>";
    html += "<h1>🔄 Factory Reset Complete</h1>";
    html += "<p style='margin-top: 15px;'>All configuration has been erased.</p>";
    html += "<p style='margin-top: 10px;'>The device will reboot now and start in setup mode.</p>";
    html += "<p style='margin-top: 15px; font-size: 14px;'>Please wait for the device to restart...</p>";
    html += "</div>";
    
    String footer = CONFIG_PORTAL_FOOTER_TEMPLATE;
    footer.replace("%VERSION%", String(FIRMWARE_VERSION));
    html += footer;
    
    html += "</div>";
    html += "</body></html>";
    
    return html;
}

String ConfigPortal::generateRebootPage() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Rebooting</title>";
    html += getCSS();
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<div class='success'>";
    html += "<h1>🔄 Rebooting</h1>";
    html += "<p style='margin-top: 15px;'>The device is restarting now.</p>";
    html += "<p style='margin-top: 10px;'>Configuration has been preserved.</p>";
    html += "<p style='margin-top: 15px; font-size: 14px;'>Please wait for the device to restart...</p>";
    html += "</div>";
    
    String footer = CONFIG_PORTAL_FOOTER_TEMPLATE;
    footer.replace("%VERSION%", String(FIRMWARE_VERSION));
    html += footer;
    
    html += "</div>";
    html += "</body></html>";
    
    return html;
}

String ConfigPortal::generateOTAPage() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Firmware Update</title>";
    html += getCSS();
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<h1>⬆️ Firmware Update</h1>";
    html += "<p class='subtitle'>Update your device firmware</p>";
    
    html += "<div class='device-info'>";
    html += "<strong>Current Version:</strong> " + String(FIRMWARE_VERSION) + "<br>";
    html += "<strong>Board:</strong> " + String(BOARD_NAME) + "<br>";
    html += "<strong>Device:</strong> " + _wifiManager->getAPName();
    html += "</div>";
    
    // Warning banner
    html += "<div class='warning-banner'>";
    html += "<strong>⚠️ Important:</strong> Do not power off the device during the update process. ";
    html += "The device will restart automatically after a successful update.";
    html += "</div>";
    
    // ========== Option 1: GitHub Releases ==========
    html += "<div style='margin-top: 30px; padding: 20px; border: 2px solid #e0e0e0; border-radius: 8px;'>";
    html += "<h2 style='margin-top: 0;'>📦 Option 1: Update from GitHub Releases</h2>";
    html += "<p style='color: #666; margin-bottom: 20px;'>Check for and install the latest firmware directly from GitHub.</p>";
    
    // Loading indicator (show by default since we auto-check)
    html += "<div id='checkLoading' style='text-align: center; margin-top: 15px; color: #666;'>";
    html += "<div style='display: inline-block; margin-right: 10px;'>⏳</div> Checking GitHub...";
    html += "</div>";
    
    // Check button (shown only if auto-check fails)
    html += "<button type='button' id='checkUpdateBtn' class='btn-primary' style='display: none; width: 100%; margin-top: 15px;' onclick='checkForUpdates()'>Retry Check</button>";
    
    // Results section
    html += "<div id='updateResults' style='display: none; margin-top: 20px;'>";
    html += "<div id='updateInfo' style='padding: 15px; background: #f5f5f5; border-radius: 6px; margin-bottom: 15px;'></div>";
    html += "<button type='button' id='installUpdateBtn' style='display: none; width: 100%;' class='btn-primary' onclick='installUpdate()'>Install Update</button>";
    html += "</div>";
    
    // Error display for GitHub check
    html += "<div id='checkError' style='display: none; margin-top: 15px; padding: 12px; background: #ffe6e6; border-left: 4px solid #cc0000; border-radius: 4px;'>";
    html += "<strong>Error:</strong> <span id='checkErrorText'></span>";
    html += "</div>";
    
    html += "</div>";
    
    // ========== Option 2: Manual Upload ==========
    html += "<div style='margin-top: 30px; padding: 20px; border: 2px solid #e0e0e0; border-radius: 8px;'>";
    html += "<h2 style='margin-top: 0;'>📁 Option 2: Upload Local Firmware File</h2>";
    html += "<p style='color: #666; margin-bottom: 20px;'>Upload a firmware file (.bin) from your computer.</p>";
    
    // Upload form
    html += "<form id='otaForm' method='POST' action='/ota' enctype='multipart/form-data'>";
    html += "<div class='form-group'>";
    html += "<label for='update'>Select Firmware File (.bin)</label>";
    html += "<input type='file' id='update' name='update' accept='.bin' required>";
    html += "<div class='help-text'>Choose a .bin firmware file for your device</div>";
    html += "</div>";
    
    // Progress bar for file upload
    html += "<div id='progressSection' style='display: none;'>";
    html += "<div class='progress-container'>";
    html += "<div class='progress-bar' id='progressBar'>0%</div>";
    html += "</div>";
    html += "<div id='statusText' class='help-text' style='text-align: center; margin-top: 10px;'></div>";
    html += "</div>";
    
    // Buttons
    html += "<div style='display: flex; gap: 10px; margin-top: 25px;'>";
    html += "<button type='submit' id='uploadBtn' class='btn-primary' style='flex: 1;'>Upload & Install</button>";
    html += "</div>";
    html += "</form>";
    
    html += "</div>";
    
    // ========== Global Status Messages ==========
    html += "<div id='installProgress' style='display: none; margin-top: 20px; padding: 20px; background: #e8f4f8; border-radius: 8px; text-align: center;'>";
    html += "<h3 style='margin-top: 0;'>Installing Firmware...</h3>";
    html += "<p id='installStatus'>Downloading from GitHub...</p>";
    html += "<div class='progress-container' style='margin-top: 15px;'>";
    html += "<div class='progress-bar' id='installProgressBar'>0%</div>";
    html += "</div>";
    html += "<p style='margin-top: 15px; color: #cc0000;'><strong>⚠️ Do not power off the device!</strong></p>";
    html += "</div>";
    
    // Success message
    html += "<div id='successMessage' class='success' style='display: none; margin-top: 20px;'>";
    html += "<h2>✅ Update Successful!</h2>";
    html += "<p>Firmware updated successfully. Device is restarting...</p>";
    html += "</div>";
    
    // Error message
    html += "<div id='errorMessage' class='error' style='display: none; margin-top: 20px;'>";
    html += "<h2>❌ Update Failed</h2>";
    html += "<p id='errorText'></p>";
    html += "</div>";
    
    // Back button
    html += "<div style='margin-top: 30px;'>";
    html += "<button type='button' class='btn-secondary' style='width: 100%;' onclick='window.location.href=\"/\"'>← Back to Configuration</button>";
    html += "</div>";
    
    // Footer
    String footer = CONFIG_PORTAL_FOOTER_TEMPLATE;
    footer.replace("%VERSION%", String(FIRMWARE_VERSION));
    html += footer;
    
    html += "</div>"; // Close container
    
    // ========== JavaScript ==========
    html += "<script>";
    
    // Global variables
    html += "var updateAssetUrl = '';";
    
    // Check for updates function
    html += "function checkForUpdates() {";
    html += "  document.getElementById('checkUpdateBtn').disabled = true;";
    html += "  document.getElementById('checkLoading').style.display = 'block';";
    html += "  document.getElementById('updateResults').style.display = 'none';";
    html += "  document.getElementById('checkError').style.display = 'none';";
    html += "  fetch('/ota/check')";
    html += "    .then(function(response) { return response.json(); })";
    html += "    .then(function(data) {";
    html += "      document.getElementById('checkLoading').style.display = 'none';";
    html += "      document.getElementById('checkUpdateBtn').disabled = false;";
    html += "      if (data.success && data.found) {";
    html += "        var infoHtml = '<strong>Current Version:</strong> ' + data.current_version + '<br>';";
    html += "        infoHtml += '<strong>Latest Version:</strong> ' + data.latest_version + '<br>';";
    html += "        infoHtml += '<strong>Asset:</strong> ' + data.asset_name + '<br>';";
    html += "        infoHtml += '<strong>Size:</strong> ' + Math.round(data.asset_size / 1024) + ' KB<br>';";
    html += "        if (data.is_newer) {";
    html += "          infoHtml += '<div style=\"margin-top: 10px; padding: 8px; background: #d4edda; border-radius: 4px; color: #155724;\"><strong>✓ Update Available</strong></div>';";
    html += "          document.getElementById('installUpdateBtn').style.display = 'block';";
    html += "          updateAssetUrl = data.asset_url;";
    html += "        } else {";
    html += "          infoHtml += '<div style=\"margin-top: 10px; padding: 8px; background: #d1ecf1; border-radius: 4px; color: #0c5460;\"><strong>✓ You are up to date</strong></div>';";
    html += "          document.getElementById('installUpdateBtn').style.display = 'none';";
    html += "        }";
    html += "        document.getElementById('updateInfo').innerHTML = infoHtml;";
    html += "        document.getElementById('updateResults').style.display = 'block';";
    html += "      } else {";
    html += "        document.getElementById('checkErrorText').innerText = data.error || 'Unknown error';";
    html += "        document.getElementById('checkError').style.display = 'block';";
    html += "      }";
    html += "    })";
    html += "    .catch(function(error) {";
    html += "      document.getElementById('checkLoading').style.display = 'none';";
    html += "      document.getElementById('checkUpdateBtn').style.display = 'block';";
    html += "      document.getElementById('checkErrorText').innerText = 'Network error: ' + error.message;";
    html += "      document.getElementById('checkError').style.display = 'block';";
    html += "    });";
    html += "}";
    
    // Auto-check on page load
    html += "window.addEventListener('DOMContentLoaded', function() {";
    html += "  checkForUpdates();";
    html += "});";
    
    // Install update function
    html += "function installUpdate() {";
    html += "  if (!updateAssetUrl) {";
    html += "    alert('No update URL available');";
    html += "    return;";
    html += "  }";
    html += "  window.location.href = '/ota/status?asset_url=' + encodeURIComponent(updateAssetUrl);";
    html += "}";
    
    // Manual file upload handler
    html += "document.getElementById('otaForm').addEventListener('submit', function(e) {";
    html += "  e.preventDefault();";
    html += "  var fileInput = document.getElementById('update');";
    html += "  var file = fileInput.files[0];";
    html += "  if (!file) {";
    html += "    alert('Please select a firmware file.');";
    html += "    return;";
    html += "  }";
    html += "  if (!file.name.endsWith('.bin')) {";
    html += "    alert('Please select a valid .bin firmware file.');";
    html += "    return;";
    html += "  }";
    html += "  var formData = new FormData();";
    html += "  formData.append('update', file);";
    html += "  var xhr = new XMLHttpRequest();";
    html += "  document.getElementById('uploadBtn').disabled = true;";
    html += "  document.getElementById('progressSection').style.display = 'block';";
    html += "  document.getElementById('statusText').innerText = 'Uploading...';";
    html += "  xhr.upload.addEventListener('progress', function(e) {";
    html += "    if (e.lengthComputable) {";
    html += "      var percentComplete = Math.round((e.loaded / e.total) * 100);";
    html += "      document.getElementById('progressBar').style.width = percentComplete + '%';";
    html += "      document.getElementById('progressBar').innerText = percentComplete + '%';";
    html += "    }";
    html += "  });";
    html += "  xhr.addEventListener('load', function() {";
    html += "    if (xhr.status === 200) {";
    html += "      document.getElementById('statusText').innerText = 'Upload complete! Flashing...';";
    html += "      document.getElementById('otaForm').style.display = 'none';";
    html += "      document.getElementById('successMessage').style.display = 'block';";
    html += "      setTimeout(function() { window.location.href = '/'; }, 10000);";
    html += "    } else {";
    html += "      document.getElementById('errorText').innerText = 'Server returned error: ' + xhr.status;";
    html += "      document.getElementById('errorMessage').style.display = 'block';";
    html += "      document.getElementById('uploadBtn').disabled = false;";
    html += "      document.getElementById('progressSection').style.display = 'none';";
    html += "    }";
    html += "  });";
    html += "  xhr.addEventListener('error', function() {";
    html += "    document.getElementById('errorText').innerText = 'Upload failed. Please try again.';";
    html += "    document.getElementById('errorMessage').style.display = 'block';";
    html += "    document.getElementById('uploadBtn').disabled = false;";
    html += "    document.getElementById('progressSection').style.display = 'none';";
    html += "  });";
    html += "  xhr.open('POST', '/ota');";
    html += "  xhr.send(formData);";
    html += "});";
    html += "</script>";
    
    html += "</body></html>";
    
    return html;
}

void ConfigPortal::handleOTA() {
    _server->send(200, "text/html", generateOTAPage());
}

void ConfigPortal::handleOTAUpload() {
    // This is called after the upload handler finishes
    if (Update.hasError()) {
        String errorMsg = "Update failed. Error #" + String(Update.getError());
        Update.printError(Serial);
        _server->send(500, "text/plain", errorMsg);
    } else {
        _server->send(200, "text/plain", "Update successful! Rebooting...");
        delay(1000);
        ESP.restart();
    }
}

void ConfigPortal::handleOTACheck() {
    LogBox::begin("OTA Check");
    LogBox::line("Checking GitHub for updates...");
    LogBox::end();
    
    GitHubOTA ota;
    GitHubOTA::ReleaseInfo info;
    
    String boardName = String(BOARD_NAME);
    bool success = ota.checkLatestRelease(boardName, info);
    
    // Build JSON response
    String json = "{";
    json += "\"success\":" + String(success ? "true" : "false") + ",";
    
    if (success) {
        json += "\"current_version\":\"" + String(FIRMWARE_VERSION) + "\",";
        json += "\"latest_version\":\"" + info.version + "\",";
        json += "\"tag_name\":\"" + info.tagName + "\",";
        json += "\"asset_name\":\"" + info.assetName + "\",";
        json += "\"asset_url\":\"" + info.assetUrl + "\",";
        json += "\"asset_size\":" + String(info.assetSize) + ",";
        json += "\"published_at\":\"" + info.publishedAt + "\",";
        json += "\"found\":" + String(info.found ? "true" : "false") + ",";
        json += "\"is_newer\":" + String(GitHubOTA::isNewerVersion(String(FIRMWARE_VERSION), info.version) ? "true" : "false");
    } else {
        json += "\"error\":\"" + ota.getLastError() + "\"";
    }
    
    json += "}";
    
    _server->send(success ? 200 : 500, "application/json", json);
}

// Task data structure for OTA updates
struct OTATaskData {
    String assetUrl;
    DisplayManager* displayManager;
    volatile bool* taskComplete;
    volatile bool* taskSuccess;
    String* errorMessage;
};

// Task function that runs OTA update with larger stack
void otaUpdateTask(void* parameter) {
    OTATaskData* data = (OTATaskData*)parameter;
    
    LogBox::begin("OTA Task");
    LogBox::line("Running OTA update in separate task");
    LogBox::line("URL: " + data->assetUrl);
    LogBox::end();
    
    // Perform the download and installation
    GitHubOTA ota;
    bool success = ota.downloadAndInstall(data->assetUrl, nullptr);
    
    if (success) {
        // Show success message on display
        if (data->displayManager != nullptr) {
            data->displayManager->clear();
            int y = MARGIN;
            data->displayManager->showMessage("Update Complete!", MARGIN, y, FONT_HEADING1);
            y += data->displayManager->getFontHeight(FONT_HEADING1) + LINE_SPACING * 2;
            data->displayManager->showMessage("Device will reboot now...", MARGIN, y, FONT_NORMAL);
            data->displayManager->refresh();
        }
        
        LogBox::begin("OTA Success");
        LogBox::line("Rebooting in 3 seconds...");
        LogBox::end();
        
        delay(3000);
        
        // Clean up before reboot
        delete data;
        
        ESP.restart();
    } else {
        // Show error message on display
        if (data->displayManager != nullptr) {
            data->displayManager->clear();
            int y = MARGIN;
            data->displayManager->showMessage("Update Failed", MARGIN, y, FONT_HEADING1);
            y += data->displayManager->getFontHeight(FONT_HEADING1) + LINE_SPACING * 2;
            data->displayManager->showMessage(ota.getLastError().c_str(), MARGIN, y, FONT_NORMAL);
            data->displayManager->refresh();
        }
        
        LogBox::begin("OTA Error");
        LogBox::line(ota.getLastError());
        LogBox::end();
        
        // Clean up
        delete data;
    }
    
    // Task will delete itself
    vTaskDelete(NULL);
}

void ConfigPortal::handleOTAInstall() {
    String assetUrl = _server->arg("asset_url");
    
    if (assetUrl.length() == 0) {
        _server->send(400, "application/json", "{\"success\":false,\"error\":\"Missing asset_url parameter\"}");
        return;
    }
    
    LogBox::begin("OTA Install");
    LogBox::line("Starting GitHub OTA update...");
    LogBox::line("URL: " + assetUrl);
    LogBox::end();
    
    // Show visual feedback on screen (same pattern as manual OTA upload)
    if (_displayManager != nullptr) {
        _displayManager->clear();
        // Draw logo at top area for visual branding
        int screenWidth = _displayManager->getWidth();
        int minLogoX = MARGIN;
        int maxLogoX = screenWidth - LOGO_WIDTH - MARGIN;
        int logoX;
        if (maxLogoX <= minLogoX) {
            logoX = minLogoX;
        } else {
            logoX = minLogoX + (maxLogoX - minLogoX) / 2;
        }
        int logoY = MARGIN;
#ifndef DISPLAY_MODE_INKPLATE2
        _displayManager->drawBitmap(logo_bitmap, logoX, logoY, LOGO_WIDTH, LOGO_HEIGHT);
#endif
        int y = logoY + LOGO_HEIGHT + MARGIN;
        _displayManager->showMessage("Firmware Update", MARGIN, y, FONT_HEADING1);
        y += _displayManager->getFontHeight(FONT_HEADING1) + LINE_SPACING * 2;
        _displayManager->showMessage("Downloading from GitHub...", MARGIN, y, FONT_NORMAL);
        y += _displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
        _displayManager->showMessage("Device will reboot when complete.", MARGIN, y, FONT_NORMAL);
        y += _displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING * 2;
        _displayManager->showMessage("Do not power off!", MARGIN, y, FONT_NORMAL);
        _displayManager->refresh();
    }
    
    // Allocate task data on heap (will be freed by task)
    OTATaskData* taskData = new OTATaskData();
    taskData->assetUrl = assetUrl;
    taskData->displayManager = _displayManager;
    taskData->taskComplete = nullptr;  // Not needed anymore
    taskData->taskSuccess = nullptr;   // Not needed anymore
    taskData->errorMessage = nullptr;  // Not needed anymore
    
    // Create task with 16KB stack (much larger than default 4KB)
    TaskHandle_t otaTask;
    xTaskCreate(
        otaUpdateTask,      // Task function
        "OTA_Update",       // Task name
        16384,              // Stack size in bytes (16KB)
        taskData,           // Parameters
        1,                  // Priority
        &otaTask            // Task handle
    );
    
    // Send immediate response and return (don't block)
    _server->send(200, "application/json", "{\"success\":true,\"message\":\"Download started...\"}");
    
    // The task will handle the rest (display updates, reboot, etc.)
}

#ifndef DISPLAY_MODE_INKPLATE2
// VCOM management handlers (not available on Inkplate 2 - no TPS65186 PMIC)

// VCOM management page handler
void ConfigPortal::handleVcom() {
    double currentVcom = NAN;
    if (_displayManager) currentVcom = _displayManager->readPanelVCOM();
    String page = generateVcomPage(currentVcom);
    _server->send(200, "text/html", page);
    
    // Display test pattern on device screen
    if (_displayManager) {
        _displayManager->showVcomTestPattern();
    }
}

// VCOM programming POST handler
void ConfigPortal::handleVcomSubmit() {
    String vcomStr = _server->arg("vcom");
    String confirm = _server->arg("confirm");
    double currentVcom = NAN;
    if (_displayManager) currentVcom = _displayManager->readPanelVCOM();
    String message;
    if (confirm != "on") {
        message = "<span style='color:red;'>You must check the confirmation box to proceed.</span>";
        String page = generateVcomPage(currentVcom, message);
        _server->send(200, "text/html", page);
        return;
    }
    double vcom = vcomStr.toDouble();
    if (vcom < -3.3 || vcom > 0) {
        message = "<span style='color:red;'>Invalid VCOM value. Must be between -3.3V and 0V.</span>";
        String page = generateVcomPage(currentVcom, message);
        _server->send(200, "text/html", page);
        return;
    }
    bool ok = false;
    String diagnostics;
    if (_displayManager) ok = _displayManager->programPanelVCOM(vcom, &diagnostics);
    if (ok) {
        message = "<span style='color:green;'>VCOM programmed successfully. New value: " + String(vcom, 3) + " V</span>";
    } else {
        message = "<span style='color:red;'>Failed to program VCOM. See diagnostics below.</span>";
    }
    if (_displayManager) currentVcom = _displayManager->readPanelVCOM();
    String page = generateVcomPage(currentVcom, message, diagnostics);
    _server->send(200, "text/html", page);
    
    // Display updated test pattern on device screen
    if (_displayManager) {
        _displayManager->showVcomTestPattern();
    }
}

// VCOM management HTML page
String ConfigPortal::generateVcomPage(double currentVcom, const String& message, const String& diagnostics) {
    String html = "<html><head><meta charset='UTF-8'><title>VCOM Management</title><style>" + getCSS() + "</style></head><body>";
    html += "<div class='container'>";
    html += "<h2>VCOM Management</h2>";
    html += "<p style='color:red;'><b>Warning:</b> Changing the VCOM value can permanently damage your e-ink display if set incorrectly. Only proceed if you know what you are doing!<br>Programming VCOM writes to the PMIC EEPROM.</p>";
    
    // Test pattern info box
    html += "<div style='margin: 15px 0; padding: 12px; background: #e8f4f8; border-left: 4px solid #0066cc; border-radius: 4px;'>";
    html += "<strong>📊 Test Pattern:</strong> Your device is now displaying a test pattern with 8 grayscale bars. ";
    html += "Compare the visual quality as you adjust VCOM values. Look for smooth gradients and good contrast.";
    html += "</div>";
    
    if (!isnan(currentVcom)) {
        html += "<p>Current VCOM value: <b>" + String(currentVcom, 3) + " V</b></p>";
    } else {
        html += "<p>Current VCOM value: <b>Unavailable</b></p>";
    }
    if (!message.isEmpty()) {
        html += "<div>" + message + "</div>";
    }
    html += "<form method='POST' action='/vcom'>";
    html += "<label for='vcom'>New VCOM value (V, -3.3 to 0): </label>";
    html += "<input type='number' id='vcom' name='vcom' step='0.001' min='-3.3' max='0' required>";
    html += "<br><input type='checkbox' id='confirm' name='confirm'> <label for='confirm'><b>I understand the risks and want to program VCOM</b></label><br>";
    html += "<button type='submit' class='btn-primary' style='width:100%;'>Program VCOM</button>";
    html += "</form>";
    
    
    if (!diagnostics.isEmpty()) {
        html += "<div style='margin-top: 20px; padding: 10px; background: #f9f9f9; border-radius: 6px; font-family: monospace; font-size: 12px;'>";
        html += "<strong>Diagnostics:</strong><br>" + diagnostics;
        html += "</div>";
    }
    html += "<div style='margin-top:20px;'><a href='/' style='text-decoration:none;display:block;'><button type='button' class='btn-secondary' style='width:100%;'>Back to main config</button></a></div>";
    html += "</div></body></html>";
    return html;
}

#endif // DISPLAY_MODE_INKPLATE2

String ConfigPortal::generateOTAStatusPage() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Updating Firmware</title>";
    html += getCSS();
    html += "<style>";
    html += ".spinner { border: 4px solid #f3f3f3; border-top: 4px solid #0066cc; border-radius: 50%; width: 40px; height: 40px; animation: spin 1s linear infinite; margin: 20px auto; }";
    html += "@keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }";
    html += ".status-box { padding: 30px; background: #e8f4f8; border-radius: 8px; text-align: center; margin: 20px 0; }";
    html += ".warning-box { padding: 15px; background: #fff3cd; border-left: 4px solid #ffc107; border-radius: 4px; margin: 20px 0; }";
    html += "</style>";
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<h1>🔄 Firmware Update</h1>";
    
    html += "<div class='status-box'>";
    html += "<div class='spinner'></div>";
    html += "<h2 id='statusTitle'>Downloading Firmware...</h2>";
    html += "<p id='statusMessage'>Please wait while the firmware is being downloaded and installed.</p>";
    html += "</div>";
    
    html += "<div class='warning-box'>";
    html += "<strong>⚠️ Important:</strong> Do not power off the device or close this page. ";
    html += "The device will restart automatically when the update is complete.";
    html += "</div>";
    
    html += "<div id='progressSection' style='margin-top: 20px;'>";
    html += "<div class='progress-container'>";
    html += "<div class='progress-bar' id='progressBar'>0%</div>";
    html += "</div>";
    html += "<p style='text-align: center; margin-top: 10px; color: #666;' id='progressText'>Initializing...</p>";
    html += "</div>";
    
    html += "<div id='errorSection' style='display: none;'>";
    html += "<div class='error'>";
    html += "<h2>❌ Update Failed</h2>";
    html += "<p id='errorMessage'></p>";
    html += "<button type='button' class='btn-primary' style='margin-top: 20px;' onclick='window.location.href=\"/ota\"'>← Back to Firmware Update</button>";
    html += "</div>";
    html += "</div>";
    
    // Footer
    String footer = CONFIG_PORTAL_FOOTER_TEMPLATE;
    footer.replace("%VERSION%", String(FIRMWARE_VERSION));
    html += footer;
    
    html += "</div>"; // Close container
    
    // JavaScript to trigger the update and poll progress
    html += "<script>";
    html += "var updateStarted = false;";
    html += "var progressInterval = null;";
    html += "var failedPolls = 0;";
    html += "function updateProgress() {";
    html += "  fetch('/ota/progress')";
    html += "    .then(function(response) { return response.json(); })";
    html += "    .then(function(data) {";
    html += "      failedPolls = 0;";
    html += "      if (data.inProgress) {";
    html += "        var percent = data.percentComplete;";
    html += "        var kb = Math.round(data.bytesDownloaded / 1024);";
    html += "        var totalKb = Math.round(data.totalBytes / 1024);";
    html += "        document.getElementById('progressBar').style.width = percent + '%';";
    html += "        document.getElementById('progressBar').innerText = percent + '%';";
    html += "        if (totalKb > 0) {";
    html += "          document.getElementById('progressText').innerText = kb + ' KB / ' + totalKb + ' KB';";
    html += "        }";
    html += "      } else if (data.percentComplete === 100) {";
    html += "        clearInterval(progressInterval);";
    html += "        document.getElementById('statusTitle').innerText = 'Installing...';";
    html += "        document.getElementById('statusMessage').innerText = 'Firmware downloaded. Installing and rebooting...';";
    html += "        document.getElementById('progressBar').style.width = '100%';";
    html += "        document.getElementById('progressBar').innerText = '100%';";
    html += "      }";
    html += "    })";
    html += "    .catch(function(error) {";
    html += "      failedPolls++;";
    html += "      if (failedPolls >= 3) {";
    html += "        clearInterval(progressInterval);";
    html += "        document.querySelector('.spinner').style.display = 'none';";
    html += "        document.getElementById('statusTitle').innerText = 'Device is Rebooting';";
    html += "        document.getElementById('statusMessage').innerText = 'The firmware has been installed successfully. The device is now rebooting...';";
    html += "        document.getElementById('progressBar').style.width = '100%';";
    html += "        document.getElementById('progressBar').innerText = '100%';";
    html += "        document.getElementById('progressText').innerText = 'Complete';";
    html += "      }";
    html += "    });";
    html += "}";
    html += "window.addEventListener('DOMContentLoaded', function() {";
    html += "  var urlParams = new URLSearchParams(window.location.search);";
    html += "  var assetUrl = urlParams.get('asset_url');";
    html += "  if (!assetUrl) {";
    html += "    document.getElementById('statusTitle').innerText = 'Error';";
    html += "    document.getElementById('statusMessage').innerText = 'Missing update URL';";
    html += "    document.querySelector('.spinner').style.display = 'none';";
    html += "    return;";
    html += "  }";
    html += "  if (updateStarted) return;";
    html += "  updateStarted = true;";
    html += "  progressInterval = setInterval(updateProgress, 500);";
    html += "  var formData = new FormData();";
    html += "  formData.append('asset_url', assetUrl);";
    html += "  fetch('/ota/install', { method: 'POST', body: formData })";
    html += "    .then(function(response) { return response.json(); })";
    html += "    .then(function(data) {";
    html += "      if (data.success) {";
    html += "        document.getElementById('statusTitle').innerText = 'Downloading...';";
    html += "        document.getElementById('statusMessage').innerText = 'Firmware is being downloaded and installed. The device will reboot automatically when complete.';";
    html += "      } else {";
    html += "        clearInterval(progressInterval);";
    html += "        document.querySelector('.status-box').style.display = 'none';";
    html += "        document.getElementById('errorMessage').innerText = data.error || 'Unknown error';";
    html += "        document.getElementById('errorSection').style.display = 'block';";
    html += "      }";
    html += "    })";
    html += "    .catch(function(error) {";
    html += "      clearInterval(progressInterval);";
    html += "      document.querySelector('.status-box').style.display = 'none';";
    html += "      document.getElementById('errorMessage').innerText = 'Network error: ' + error.message;";
    html += "      document.getElementById('errorSection').style.display = 'block';";
    html += "    });";
    html += "});";
    html += "</script>";
    
    html += "</body></html>";
    
    return html;
}

void ConfigPortal::handleOTAStatus() {
    _server->send(200, "text/html", generateOTAStatusPage());
}

void ConfigPortal::handleOTAProgress() {
    // Return JSON with current progress
    String json = "{";
    json += "\"inProgress\":" + String(g_otaProgress.inProgress ? "true" : "false") + ",";
    json += "\"bytesDownloaded\":" + String(g_otaProgress.bytesDownloaded) + ",";
    json += "\"totalBytes\":" + String(g_otaProgress.totalBytes) + ",";
    json += "\"percentComplete\":" + String(g_otaProgress.percentComplete);
    json += "}";
    
    _server->send(200, "application/json", json);
}
