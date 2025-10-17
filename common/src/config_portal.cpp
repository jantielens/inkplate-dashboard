#include "config_portal.h"
#include "config_portal_css.h"
#include "version.h"
#include "config.h"

ConfigPortal::ConfigPortal(ConfigManager* configManager, WiFiManager* wifiManager, DisplayManager* displayManager)
    : _configManager(configManager), _wifiManager(wifiManager), _displayManager(displayManager),
      _server(nullptr), _configReceived(false), _port(80), _mode(CONFIG_MODE) {
}

ConfigPortal::~ConfigPortal() {
    stop();
}

bool ConfigPortal::begin(PortalMode mode) {
    if (_server != nullptr) {
        Serial.println("Config portal already running");
        return true;
    }
    
    _mode = mode;
    _server = new WebServer(_port);
    
    // Set up routes
    _server->on("/", [this]() { this->handleRoot(); });
    _server->on("/submit", HTTP_POST, [this]() { this->handleSubmit(); });
    _server->on("/factory-reset", HTTP_POST, [this]() { this->handleFactoryReset(); });
    
    // OTA routes - only available in CONFIG_MODE
    if (_mode == CONFIG_MODE) {
        _server->on("/ota", HTTP_GET, [this]() { this->handleOTA(); });
        _server->on("/ota", HTTP_POST, 
            [this]() { this->handleOTAUpload(); },
            [this]() { 
                // Handle upload data
                HTTPUpload& upload = _server->upload();
                if (upload.status == UPLOAD_FILE_START) {
                    Serial.printf("OTA Update: %s\n", upload.filename.c_str());
                    
                    // Show visual feedback on screen
                    if (_displayManager != nullptr) {
                        _displayManager->clear();
                        int y = MARGIN;
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
                        Serial.printf("OTA Update Success: %u bytes\n", upload.totalSize);
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
    Serial.println("Configuration portal started on port " + String(_port));
    Serial.println("Open http://" + _wifiManager->getAPIPAddress() + " in your browser");
    
    return true;
}

void ConfigPortal::stop() {
    if (_server != nullptr) {
        _server->stop();
        delete _server;
        _server = nullptr;
        Serial.println("Configuration portal stopped");
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
    Serial.println("Serving configuration page");
    _server->send(200, "text/html", generateConfigPage());
}

void ConfigPortal::handleSubmit() {
    Serial.println("Configuration form submitted");
    
    // Get form data
    String ssid = _server->arg("ssid");
    String password = _server->arg("password");
    String imageUrl = _server->arg("imageurl");
    String refreshRateStr = _server->arg("refresh");
    String mqttBroker = _server->arg("mqttbroker");
    String mqttUser = _server->arg("mqttuser");
    String mqttPass = _server->arg("mqttpass");
    bool debugMode = _server->hasArg("debugmode") && _server->arg("debugmode") == "on";
    bool useCRC32Check = _server->hasArg("crc32check") && _server->arg("crc32check") == "on";
    
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
        Serial.println("WiFi credentials saved (boot mode)");
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
    
    // Handle WiFi password - if empty and device is configured, keep existing password
    if (password.length() == 0 && _configManager->isConfigured()) {
        config.wifiPassword = _configManager->getWiFiPassword();
        Serial.println("Keeping existing WiFi password");
    } else {
        config.wifiPassword = password;
    }
    
    // Handle MQTT password - if empty and device is configured with MQTT, keep existing password
    if (mqttPass.length() == 0 && _configManager->isConfigured() && _configManager->getMQTTPassword().length() > 0) {
        config.mqttPassword = _configManager->getMQTTPassword();
        Serial.println("Keeping existing MQTT password");
    } else {
        config.mqttPassword = mqttPass;
    }
    
    // Save configuration
    if (_configManager->saveConfig(config)) {
        Serial.println("Configuration saved successfully");
        _configReceived = true;
        _server->send(200, "text/html", generateSuccessPage());
    } else {
        Serial.println("Failed to save configuration");
        _server->send(500, "text/html", generateErrorPage("Failed to save configuration"));
    }
}

void ConfigPortal::handleFactoryReset() {
    Serial.println("Factory reset requested");
    
    // Clear all configuration
    _configManager->clearConfig();
    
    // Send success page
    _server->send(200, "text/html", generateFactoryResetPage());
    
    // Device will reboot after the response is sent
    Serial.println("Factory reset completed. Device will reboot in 2 seconds...");
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
        
        // Refresh Rate - only shown in CONFIG_MODE
        html += "<div class='form-group'>";
        html += "<label for='refresh'>Refresh Rate (minutes)</label>";
        if (hasConfig) {
            html += "<input type='number' id='refresh' name='refresh' min='1' value='" + String(currentConfig.refreshRate) + "' placeholder='5'>";
        } else {
            html += "<input type='number' id='refresh' name='refresh' min='1' value='5' placeholder='5'>";
        }
        html += "<div class='help-text'>How often to update the image (default: 5 minutes)</div>";
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
        
        // MQTT Configuration - optional section in CONFIG_MODE
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
    }
    
    // Factory Reset Section - only show in CONFIG_MODE if device is configured
    if (_mode == CONFIG_MODE && hasConfig) {
        html += "<div class='factory-reset-section'>";
        html += "<div class='danger-zone'>";
        html += "<h2>⚠️ Danger Zone</h2>";
        html += "<p>Factory reset will erase all settings including WiFi credentials and configuration. The device will reboot and start fresh.</p>";
        html += "<button class='btn-danger' onclick='showResetModal()'>🗑️ Factory Reset</button>";
        html += "</div>";
        html += "</div>";
    }
    
    html += "</div>";
    
    // Modal dialog for factory reset confirmation (only in CONFIG_MODE)
    if (_mode == CONFIG_MODE && hasConfig) {
        html += "<div id='resetModal' class='modal'>";
        html += "<div class='modal-content'>";
        html += "<h2 style='color: #dc2626; margin-bottom: 15px;'>⚠️ Confirm Factory Reset</h2>";
        html += "<p style='color: #666; font-size: 14px; margin-bottom: 20px;'>This will permanently delete all configuration settings. Are you sure you want to continue?</p>";
        html += "<div class='modal-buttons'>";
        html += "<button class='btn-cancel' onclick='hideResetModal()'>Cancel</button>";
        html += "<form action='/factory-reset' method='POST' style='flex: 1; margin: 0;'>";
        html += "<button type='submit' class='btn-confirm' style='width: 100%;'>Yes, Reset</button>";
        html += "</form>";
        html += "</div>";
        html += "</div>";
        html += "</div>";
        
        // JavaScript for modal
        html += "<script>";
        html += "function showResetModal() { document.getElementById('resetModal').style.display = 'block'; }";
        html += "function hideResetModal() { document.getElementById('resetModal').style.display = 'none'; }";
        html += "window.onclick = function(event) { var modal = document.getElementById('resetModal'); if (event.target == modal) { modal.style.display = 'none'; } }";
        html += "</script>";
    }
    
    // Footer with version
    html += "<div style='text-align: center; margin-top: 30px; padding-top: 20px; border-top: 1px solid #e0e0e0; color: #999; font-size: 12px;'>";
    html += "Inkplate Dashboard v" + String(FIRMWARE_VERSION);
    html += "</div>";
    
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
    html += "<div style='text-align: center; margin-top: 30px; padding-top: 20px; border-top: 1px solid #e0e0e0; color: #999; font-size: 12px;'>";
    html += "Inkplate Dashboard v" + String(FIRMWARE_VERSION);
    html += "</div>";
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
    html += "<div style='text-align: center; margin-top: 30px; padding-top: 20px; border-top: 1px solid #e0e0e0; color: #999; font-size: 12px;'>";
    html += "Inkplate Dashboard v" + String(FIRMWARE_VERSION);
    html += "</div>";
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
    html += "<div style='text-align: center; margin-top: 30px; padding-top: 20px; border-top: 1px solid #e0e0e0; color: #999; font-size: 12px;'>";
    html += "Inkplate Dashboard v" + String(FIRMWARE_VERSION);
    html += "</div>";
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
    html += "<p class='subtitle'>Upload new firmware to your device</p>";
    
    html += "<div class='device-info'>";
    html += "<strong>Current Version:</strong> " + String(FIRMWARE_VERSION) + "<br>";
    html += "<strong>Device:</strong> " + _wifiManager->getAPName();
    html += "</div>";
    
    // Warning banner
    html += "<div class='warning-banner'>";
    html += "<strong>⚠️ Important:</strong> Only upload firmware files (.bin) built for your specific Inkplate model. ";
    html += "The device will restart after a successful update. Do not power off during the update process.";
    html += "</div>";
    
    // Upload form
    html += "<form id='otaForm' method='POST' action='/ota' enctype='multipart/form-data'>";
    html += "<div class='form-group'>";
    html += "<label for='update'>Select Firmware File (.bin)</label>";
    html += "<input type='file' id='update' name='update' accept='.bin' required>";
    html += "<div class='help-text'>Choose a .bin firmware file for your device</div>";
    html += "</div>";
    
    // Progress bar
    html += "<div id='progressSection' style='display: none;'>";
    html += "<div class='progress-container'>";
    html += "<div class='progress-bar' id='progressBar'>0%</div>";
    html += "</div>";
    html += "<div id='statusText' class='help-text' style='text-align: center; margin-top: 10px;'></div>";
    html += "</div>";
    
    // Buttons
    html += "<div style='display: flex; gap: 10px; margin-top: 25px;'>";
    html += "<button type='button' class='btn-cancel' onclick='window.location.href=\"/\"'>Cancel</button>";
    html += "<button type='submit' id='uploadBtn' class='btn-primary' style='flex: 1;'>Upload Firmware</button>";
    html += "</div>";
    html += "</form>";
    
    // Success/Error messages
    html += "<div id='successMessage' class='success' style='display: none; margin-top: 20px;'>";
    html += "<h2>✅ Upload Successful!</h2>";
    html += "<p>Firmware updated successfully. Device is restarting...</p>";
    html += "</div>";
    
    html += "<div id='errorMessage' class='error' style='display: none; margin-top: 20px;'>";
    html += "<h2>❌ Upload Failed</h2>";
    html += "<p id='errorText'></p>";
    html += "</div>";
    
    // Footer
    html += "<div style='text-align: center; margin-top: 30px; padding-top: 20px; border-top: 1px solid #e0e0e0; color: #999; font-size: 12px;'>";
    html += "Inkplate Dashboard v" + String(FIRMWARE_VERSION);
    html += "</div>";
    
    html += "</div>"; // Close container
    
    // JavaScript for upload handling
    html += "<script>";
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

