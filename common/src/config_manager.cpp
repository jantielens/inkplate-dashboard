#include "config_manager.h"
#include "logger.h"

ConfigManager::ConfigManager() : _initialized(false) {
}

ConfigManager::~ConfigManager() {
    if (_initialized) {
        _preferences.end();
    }
}

bool ConfigManager::begin() {
    if (_initialized) {
        return true;
    }
    
    _initialized = _preferences.begin(PREF_NAMESPACE, false);
    if (!_initialized) {
        LogBox::begin("ConfigManager Error");
        LogBox::line("Failed to initialize Preferences");
        LogBox::end();
    }
    return _initialized;
}

bool ConfigManager::isConfigured() {
    if (!_initialized && !begin()) {
        return false;
    }
    
    return _preferences.getBool(PREF_CONFIGURED, false);
}

bool ConfigManager::hasWiFiConfig() {
    if (!_initialized && !begin()) {
        return false;
    }
    
    String ssid = _preferences.getString(PREF_WIFI_SSID, "");
    return ssid.length() > 0;
}

bool ConfigManager::isFullyConfigured() {
    if (!_initialized && !begin()) {
        return false;
    }
    
    String ssid = _preferences.getString(PREF_WIFI_SSID, "");
    String imageUrl = _preferences.getString(PREF_IMAGE_URL, "");
    
    return (ssid.length() > 0 && imageUrl.length() > 0);
}

bool ConfigManager::loadConfig(DashboardConfig& config) {
    if (!_initialized && !begin()) {
        LogBox::begin("ConfigManager Error");
        LogBox::line("ConfigManager not initialized");
        LogBox::end();
        return false;
    }
    
    // Load all configuration values
    config.isConfigured = _preferences.getBool(PREF_CONFIGURED, false);
    
    if (!config.isConfigured) {
        LogBox::begin("Config Status");
        LogBox::line("Device not configured yet");
        LogBox::end();
        return false;
    }
    
    config.wifiSSID = _preferences.getString(PREF_WIFI_SSID, "");
    config.wifiPassword = _preferences.getString(PREF_WIFI_PASS, "");
    config.imageURL = _preferences.getString(PREF_IMAGE_URL, "");
    config.refreshRate = _preferences.getInt(PREF_REFRESH_RATE, DEFAULT_REFRESH_RATE);
    config.mqttBroker = _preferences.getString(PREF_MQTT_BROKER, "");
    config.mqttUsername = _preferences.getString(PREF_MQTT_USER, "");
    config.mqttPassword = _preferences.getString(PREF_MQTT_PASS, "");
    config.debugMode = _preferences.getBool(PREF_DEBUG_MODE, false);
    config.useCRC32Check = _preferences.getBool(PREF_USE_CRC32, false);
    
    // Validate configuration
    if (config.wifiSSID.length() == 0 || config.imageURL.length() == 0) {
        LogBox::begin("Config Error");
        LogBox::line("Invalid configuration: missing SSID or URL");
        LogBox::end();
        return false;
    }
    
    LogBox::begin("Configuration Loaded");
    LogBox::line("WiFi SSID: " + config.wifiSSID);
    LogBox::line("Image URL: " + config.imageURL);
    LogBox::linef("Refresh Rate: %d minutes", config.refreshRate);
    if (config.mqttBroker.length() > 0) {
        LogBox::line("MQTT Broker: " + config.mqttBroker);
        LogBox::line("MQTT Username: " + (config.mqttUsername.length() > 0 ? config.mqttUsername : "(none)"));
    }
    LogBox::end();
    
    return true;
}

bool ConfigManager::saveConfig(const DashboardConfig& config) {
    if (!_initialized && !begin()) {
        LogBox::begin("ConfigManager Error");
        LogBox::line("ConfigManager not initialized");
        LogBox::end();
        return false;
    }
    
    // Validate input
    if (config.wifiSSID.length() == 0) {
        LogBox::begin("Config Error");
        LogBox::line("WiFi SSID cannot be empty");
        LogBox::end();
        return false;
    }
    
    if (config.imageURL.length() == 0) {
        LogBox::begin("Config Error");
        LogBox::line("Image URL cannot be empty");
        LogBox::end();
        return false;
    }
    
    if (config.refreshRate < 1) {
        LogBox::begin("Config Error");
        LogBox::line("Refresh rate must be at least 1 minute");
        LogBox::end();
        return false;
    }
    
    // Save all configuration values
    _preferences.putString(PREF_WIFI_SSID, config.wifiSSID);
    _preferences.putString(PREF_WIFI_PASS, config.wifiPassword);
    _preferences.putString(PREF_IMAGE_URL, config.imageURL);
    _preferences.putInt(PREF_REFRESH_RATE, config.refreshRate);
    _preferences.putString(PREF_MQTT_BROKER, config.mqttBroker);
    _preferences.putString(PREF_MQTT_USER, config.mqttUsername);
    _preferences.putString(PREF_MQTT_PASS, config.mqttPassword);
    _preferences.putBool(PREF_CONFIGURED, true);
    _preferences.putBool(PREF_DEBUG_MODE, config.debugMode);
    _preferences.putBool(PREF_USE_CRC32, config.useCRC32Check);
    
    LogBox::begin("Config Saved");
    LogBox::line("Configuration saved successfully");
    LogBox::end();
    
    return true;
}

void ConfigManager::clearConfig() {
    if (!_initialized && !begin()) {
        LogBox::begin("ConfigManager Error");
        LogBox::line("ConfigManager not initialized");
        LogBox::end();
        return;
    }
    
    _preferences.clear();
    LogBox::begin("Factory Reset");
    LogBox::line("Configuration cleared (factory reset)");
    LogBox::end();
}

String ConfigManager::getWiFiSSID() {
    if (!_initialized && !begin()) {
        return "";
    }
    return _preferences.getString(PREF_WIFI_SSID, "");
}

String ConfigManager::getWiFiPassword() {
    if (!_initialized && !begin()) {
        return "";
    }
    return _preferences.getString(PREF_WIFI_PASS, "");
}

String ConfigManager::getImageURL() {
    if (!_initialized && !begin()) {
        return "";
    }
    return _preferences.getString(PREF_IMAGE_URL, "");
}

int ConfigManager::getRefreshRate() {
    if (!_initialized && !begin()) {
        return DEFAULT_REFRESH_RATE;
    }
    return _preferences.getInt(PREF_REFRESH_RATE, DEFAULT_REFRESH_RATE);
}

String ConfigManager::getMQTTBroker() {
    if (!_initialized && !begin()) {
        return "";
    }
    return _preferences.getString(PREF_MQTT_BROKER, "");
}

String ConfigManager::getMQTTUsername() {
    if (!_initialized && !begin()) {
        return "";
    }
    return _preferences.getString(PREF_MQTT_USER, "");
}

String ConfigManager::getMQTTPassword() {
    if (!_initialized && !begin()) {
        return "";
    }
    return _preferences.getString(PREF_MQTT_PASS, "");
}

bool ConfigManager::getDebugMode() {
    if (!_initialized && !begin()) {
        return false;
    }
    return _preferences.getBool(PREF_DEBUG_MODE, false);
}

void ConfigManager::setWiFiCredentials(const String& ssid, const String& password) {
    if (!_initialized && !begin()) {
        LogBox::begin("ConfigManager Error");
        LogBox::line("ConfigManager not initialized");
        LogBox::end();
        return;
    }
    
    _preferences.putString(PREF_WIFI_SSID, ssid);
    _preferences.putString(PREF_WIFI_PASS, password);
    LogBox::begin("Config Update");
    LogBox::line("WiFi credentials updated");
    LogBox::end();
}

void ConfigManager::setImageURL(const String& url) {
    if (!_initialized && !begin()) {
        LogBox::begin("ConfigManager Error");
        LogBox::line("ConfigManager not initialized");
        LogBox::end();
        return;
    }
    
    _preferences.putString(PREF_IMAGE_URL, url);
    LogBox::begin("Config Update");
    LogBox::line("Image URL updated: " + url);
    LogBox::end();
}

void ConfigManager::setRefreshRate(int minutes) {
    if (!_initialized && !begin()) {
        LogBox::begin("ConfigManager Error");
        LogBox::line("ConfigManager not initialized");
        LogBox::end();
        return;
    }
    
    if (minutes < 1) {
        LogBox::begin("Config Error");
        LogBox::line("Refresh rate must be at least 1 minute");
        LogBox::end();
        return;
    }
    
    _preferences.putInt(PREF_REFRESH_RATE, minutes);
    LogBox::begin("Config Update");
    LogBox::linef("Refresh rate updated: %d minutes", minutes);
    LogBox::end();
}

void ConfigManager::setMQTTConfig(const String& broker, const String& username, const String& password) {
    if (!_initialized && !begin()) {
        LogBox::begin("ConfigManager Error");
        LogBox::line("ConfigManager not initialized");
        LogBox::end();
        return;
    }
    
    _preferences.putString(PREF_MQTT_BROKER, broker);
    _preferences.putString(PREF_MQTT_USER, username);
    _preferences.putString(PREF_MQTT_PASS, password);
    LogBox::begin("Config Update");
    LogBox::line("MQTT configuration updated");
    LogBox::end();
}

void ConfigManager::setDebugMode(bool enabled) {
    if (!_initialized && !begin()) {
        LogBox::begin("ConfigManager Error");
        LogBox::line("ConfigManager not initialized");
        LogBox::end();
        return;
    }

    _preferences.putBool(PREF_DEBUG_MODE, enabled);
    LogBox::begin("Config Update");
    LogBox::line("Debug mode updated: " + String(enabled ? "ON" : "OFF"));
    LogBox::end();
}

void ConfigManager::setUseCRC32Check(bool enabled) {
    if (!_initialized && !begin()) {
        LogBox::begin("ConfigManager Error");
        LogBox::line("ConfigManager not initialized");
        LogBox::end();
        return;
    }

    _preferences.putBool(PREF_USE_CRC32, enabled);
    LogBox::begin("Config Update");
    LogBox::line("CRC32 check updated: " + String(enabled ? "ON" : "OFF"));
    LogBox::end();
}

bool ConfigManager::getUseCRC32Check() {
    if (!_initialized && !begin()) {
        return false;
    }
    return _preferences.getBool(PREF_USE_CRC32, false);
}

uint32_t ConfigManager::getLastCRC32() {
    if (!_initialized && !begin()) {
        return 0;
    }
    return _preferences.getUInt(PREF_LAST_CRC32, 0);
}

void ConfigManager::setLastCRC32(uint32_t crc32) {
    if (!_initialized && !begin()) {
        LogBox::line("ConfigManager not initialized - cannot save CRC32");
        return;
    }
    
    _preferences.putUInt(PREF_LAST_CRC32, crc32);
    LogBox::linef("Saved to preferences: 0x%08X", crc32);
}

void ConfigManager::markAsConfigured() {
    if (!_initialized && !begin()) {
        LogBox::begin("ConfigManager Error");
        LogBox::line("ConfigManager not initialized");
        LogBox::end();
        return;
    }
    
    _preferences.putBool(PREF_CONFIGURED, true);
    LogBox::begin("Config Update");
    LogBox::line("Device marked as configured");
    LogBox::end();
}
