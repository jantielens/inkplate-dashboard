#include "config_portal.h"
#include "version.h"

ConfigPortal::ConfigPortal(ConfigManager* configManager, WiFiManager* wifiManager)
    : _configManager(configManager), _wifiManager(wifiManager), 
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
    return R"(
<style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
        font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        min-height: 100vh;
        display: flex;
        justify-content: center;
        align-items: center;
        padding: 20px;
    }
    .container {
        background: white;
        border-radius: 12px;
        box-shadow: 0 10px 40px rgba(0,0,0,0.2);
        padding: 40px;
        max-width: 500px;
        width: 100%;
    }
    h1 {
        color: #333;
        margin-bottom: 10px;
        font-size: 28px;
    }
    .subtitle {
        color: #666;
        margin-bottom: 30px;
        font-size: 14px;
    }
    .form-group {
        margin-bottom: 20px;
    }
    label {
        display: block;
        margin-bottom: 8px;
        color: #333;
        font-weight: 500;
        font-size: 14px;
    }
    input[type="text"],
    input[type="password"],
    input[type="number"],
    input[type="url"] {
        width: 100%;
        padding: 12px;
        border: 2px solid #e0e0e0;
        border-radius: 6px;
        font-size: 14px;
        transition: border-color 0.3s;
    }
    input:focus {
        outline: none;
        border-color: #667eea;
    }
    .help-text {
        font-size: 12px;
        color: #666;
        margin-top: 5px;
    }
    button {
        width: 100%;
        padding: 14px;
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        color: white;
        border: none;
        border-radius: 6px;
        font-size: 16px;
        font-weight: 600;
        cursor: pointer;
        transition: transform 0.2s;
    }
    button:hover {
        transform: translateY(-2px);
    }
    button:active {
        transform: translateY(0);
    }
    .success {
        background: #10b981;
        color: white;
        padding: 20px;
        border-radius: 8px;
        text-align: center;
    }
    .error {
        background: #ef4444;
        color: white;
        padding: 20px;
        border-radius: 8px;
        text-align: center;
    }
    .device-info {
        background: #f3f4f6;
        padding: 15px;
        border-radius: 6px;
        margin-bottom: 20px;
        font-size: 14px;
    }
    .device-info strong {
        color: #667eea;
    }
    .factory-reset-section {
        margin-top: 30px;
        padding-top: 30px;
        border-top: 2px solid #e0e0e0;
    }
    .danger-zone {
        background: #fef2f2;
        border: 2px solid #fee2e2;
        border-radius: 8px;
        padding: 20px;
    }
    .danger-zone h2 {
        color: #dc2626;
        font-size: 18px;
        margin-bottom: 10px;
    }
    .danger-zone p {
        color: #666;
        font-size: 14px;
        margin-bottom: 15px;
    }
    .btn-danger {
        background: #dc2626;
        color: white;
        padding: 12px 24px;
        border: none;
        border-radius: 6px;
        font-size: 14px;
        font-weight: 600;
        cursor: pointer;
        transition: background 0.3s;
    }
    .btn-danger:hover {
        background: #b91c1c;
    }
    .modal {
        display: none;
        position: fixed;
        z-index: 1000;
        left: 0;
        top: 0;
        width: 100%;
        height: 100%;
        background-color: rgba(0,0,0,0.5);
    }
    .modal-content {
        background-color: white;
        margin: 15% auto;
        padding: 30px;
        border-radius: 12px;
        max-width: 400px;
        box-shadow: 0 10px 40px rgba(0,0,0,0.3);
    }
    .modal-buttons {
        display: flex;
        gap: 10px;
        margin-top: 20px;
    }
    .btn-cancel {
        flex: 1;
        background: #6b7280;
        color: white;
        padding: 12px;
        border: none;
        border-radius: 6px;
        font-size: 14px;
        font-weight: 600;
        cursor: pointer;
    }
    .btn-confirm {
        flex: 1;
        background: #dc2626;
        color: white;
        padding: 12px;
        border: none;
        border-radius: 6px;
        font-size: 14px;
        font-weight: 600;
        cursor: pointer;
    }
</style>
)";
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
