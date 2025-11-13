#include <src/modes/config_mode_controller.h>

// 5 minute timeout constant
const unsigned long CONFIG_MODE_TIMEOUT_MS = 5 * 60 * 1000;

ConfigModeController::ConfigModeController(ConfigManager* config, WiFiManager* wifi, ConfigPortal* portal,
                                           MQTTManager* mqtt, PowerManager* power,
                                           UIStatus* uiStatus, UIError* uiError)
    : configManager(config), wifiManager(wifi), configPortal(portal),
      mqttManager(mqtt), powerManager(power), uiStatus(uiStatus), uiError(uiError),
      hasPartialConfig(false) {
}

bool ConfigModeController::begin() {
    DashboardConfig config;
    hasPartialConfig = configManager->hasWiFiConfig() && !configManager->isFullyConfigured();
    
    // For partial config, we may not have a full config to load
    if (!hasPartialConfig && !configManager->loadConfig(config)) {
        LogBox::message("Config Mode Error", "Failed to load config");
        uiError->showConfigLoadError();
        delay(3000);
        
        powerManager->prepareForSleep();
        powerManager->enterDeepSleep(300.0f);  // Default 5 minutes = 300 seconds
        return false;
    }
    
    // For partial config, get WiFi credentials directly
    if (hasPartialConfig) {
        config.wifiSSID = configManager->getWiFiSSID();
        config.wifiPassword = configManager->getWiFiPassword();
        // No need to set refreshRate anymore
    }
    
    // Show config mode message (skip on slow displays to reduce screen updates)
#if DISPLAY_FAST_REFRESH
    uiStatus->showConfigModeConnecting(config.wifiSSID.c_str(), hasPartialConfig);
#endif
    
    // Connect to configured WiFi
    if (wifiManager->connectToWiFi()) {
        String localIP = wifiManager->getLocalIP();
        
        // Publish log message for config mode entry (if MQTT is configured)
        if (mqttManager->begin() && mqttManager->isConfigured()) {
            String deviceId = "inkplate-" + String((uint32_t)ESP.getEfuseMac(), HEX);
            if (mqttManager->connect()) {
                mqttManager->publishLastLog(deviceId, "Config mode entered", "info");
                mqttManager->disconnect();
            }
        }
        
        return startConfigPortalWithWiFi(localIP);
    } else {
        // WiFi connection failed - fall back to AP mode
        LogBox::begin("WiFi Failed");
        LogBox::line("WiFi connection failed in config mode");
        LogBox::line("Falling back to AP mode");
        LogBox::end();
        
        // Show WiFi failed message (skip on slow displays to reduce screen updates)
#if DISPLAY_FAST_REFRESH
        uiStatus->showConfigModeWiFiFailed(config.wifiSSID.c_str());
        delay(2000);
#endif
        
        return startConfigPortalWithAP();
    }
}

bool ConfigModeController::startConfigPortalWithWiFi(const String& localIP) {
    String mdnsHostname = wifiManager->getMDNSHostname();
    
    // Show appropriate config mode screen
    if (hasPartialConfig) {
        uiStatus->showConfigModePartialSetup(localIP.c_str(), mdnsHostname.c_str());
    } else {
        uiStatus->showConfigModeSetup(localIP.c_str(), true, CONFIG_MODE_TIMEOUT_MS / 60000, mdnsHostname.c_str());
    }
    
    // Start configuration portal in CONFIG_MODE
    if (configPortal->begin(CONFIG_MODE)) {
        LogBox::begin("Config Mode Active");
        if (mdnsHostname.length() > 0) {
            LogBox::line("Access at: http://" + mdnsHostname + " or http://" + localIP);
        } else {
            LogBox::line("Access at: http://" + localIP);
        }
        if (!hasPartialConfig) {
            LogBox::linef("Timeout: %d minutes", CONFIG_MODE_TIMEOUT_MS / 60000);
        }
        LogBox::end();
        return true;
    } else {
        LogBox::message("Portal Error", "Failed to start configuration portal");
        uiError->showPortalError();
        delay(3000);
        
        // Load average interval for sleep
        DashboardConfig config;
        uint16_t sleepMinutes = DEFAULT_INTERVAL_MINUTES;
        if (configManager->loadConfig(config)) {
            sleepMinutes = config.getAverageInterval();
        }
        
        powerManager->prepareForSleep();
        powerManager->enterDeepSleep((uint16_t)(sleepMinutes * 60));  // Convert minutes to seconds
        return false;
    }
}

bool ConfigModeController::startConfigPortalWithAP() {
    // Start AP mode as fallback
    if (wifiManager->startAccessPoint()) {
        String apName = wifiManager->getAPName();
        String apIP = wifiManager->getAPIPAddress();
        String mdnsHostname = wifiManager->getMDNSHostname();
        
        uiStatus->showConfigModeAPFallback(apName.c_str(), apIP.c_str(), !hasPartialConfig, CONFIG_MODE_TIMEOUT_MS / 60000, mdnsHostname.c_str());
        
        // Start configuration portal in CONFIG_MODE (with AP)
        if (configPortal->begin(CONFIG_MODE)) {
            LogBox::begin("Config Mode Active (AP Fallback)");
            LogBox::line("1. Connect to WiFi: " + apName);
            if (mdnsHostname.length() > 0) {
                LogBox::line("2. Open: http://" + mdnsHostname + " or http://" + apIP);
            } else {
                LogBox::line("2. Open: http://" + apIP);
            }
            LogBox::line("3. Update your configuration");
            if (!hasPartialConfig) {
                LogBox::linef("Timeout: %d minutes", CONFIG_MODE_TIMEOUT_MS / 60000);
            }
            LogBox::end();
            return true;
        } else {
            LogBox::message("AP Mode Error", "Failed to start AP mode fallback");
            uiError->showAPStartError();
            delay(3000);
            
            // Load average interval for sleep, with minimum 5 minute fallback
            DashboardConfig config;
            uint16_t sleepMinutes = DEFAULT_INTERVAL_MINUTES;
            if (configManager->loadConfig(config)) {
                uint16_t avgInterval = config.getAverageInterval();
                sleepMinutes = (avgInterval > 0) ? avgInterval : 5;
            }
            
            powerManager->prepareForSleep();
            powerManager->enterDeepSleep((float)(sleepMinutes * 60));  // Convert minutes to seconds
            return false;
        }
    } else {
        LogBox::message("AP Mode Error", "Failed to start AP mode fallback");
        uiError->showConfigModeFailure();
        delay(3000);
        
        // Load average interval for sleep, with minimum 5 minute fallback
        DashboardConfig config;
        uint16_t sleepMinutes = DEFAULT_INTERVAL_MINUTES;
        if (configManager->loadConfig(config)) {
            uint16_t avgInterval = config.getAverageInterval();
            sleepMinutes = (avgInterval > 0) ? avgInterval : 5;
        }
        
        powerManager->prepareForSleep();
        powerManager->enterDeepSleep((float)(sleepMinutes * 60));  // Convert minutes to seconds
        return false;
    }
}

void ConfigModeController::handleClient() {
    configPortal->handleClient();
}

bool ConfigModeController::isConfigReceived() {
    return configPortal->isConfigReceived();
}

bool ConfigModeController::isTimedOut(unsigned long startTime) {
    // No timeout for partial config (auto-enter mode)
    if (hasPartialConfig) {
        return false;
    }
    return (millis() - startTime > CONFIG_MODE_TIMEOUT_MS);
}

void ConfigModeController::handleTimeout(uint16_t refreshMinutes) {
    LogBox::begin("Config Timeout");
    LogBox::line("Config mode timeout");
    LogBox::line("Restarting device");
    LogBox::end();
    
    // Publish log message for config mode timeout (if MQTT is configured)
    if (mqttManager->begin() && mqttManager->isConfigured()) {
        String deviceId = "inkplate-" + String((uint32_t)ESP.getEfuseMac(), HEX);
        if (mqttManager->connect()) {
            mqttManager->publishLastLog(deviceId, "Config mode timeout - restarting", "info");
            mqttManager->disconnect();
        }
    }
    
    uiStatus->showConfigModeTimeout();
    
    // Restart device to reload configuration and display image
    // Same approach as when user saves configuration
    delay(3000);
    ESP.restart();
}

