#include "config_manager.h"

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
        Serial.println("Failed to initialize Preferences");
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
        Serial.println("ConfigManager not initialized");
        return false;
    }
    
    // Load all configuration values
    config.isConfigured = _preferences.getBool(PREF_CONFIGURED, false);
    
    if (!config.isConfigured) {
        Serial.println("Device not configured yet");
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
        Serial.println("Invalid configuration: missing SSID or URL");
        return false;
    }
    
    Serial.println("Configuration loaded successfully:");
    Serial.println("  WiFi SSID: " + config.wifiSSID);
    Serial.println("  Image URL: " + config.imageURL);
    Serial.println("  Refresh Rate: " + String(config.refreshRate) + " minutes");
    if (config.mqttBroker.length() > 0) {
        Serial.println("  MQTT Broker: " + config.mqttBroker);
        Serial.println("  MQTT Username: " + (config.mqttUsername.length() > 0 ? config.mqttUsername : "(none)"));
    }
    
    return true;
}

bool ConfigManager::saveConfig(const DashboardConfig& config) {
    if (!_initialized && !begin()) {
        Serial.println("ConfigManager not initialized");
        return false;
    }
    
    // Validate input
    if (config.wifiSSID.length() == 0) {
        Serial.println("Error: WiFi SSID cannot be empty");
        return false;
    }
    
    if (config.imageURL.length() == 0) {
        Serial.println("Error: Image URL cannot be empty");
        return false;
    }
    
    if (config.refreshRate < 1) {
        Serial.println("Error: Refresh rate must be at least 1 minute");
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
    
    Serial.println("Configuration saved successfully");
    
    return true;
}

void ConfigManager::clearConfig() {
    if (!_initialized && !begin()) {
        Serial.println("ConfigManager not initialized");
        return;
    }
    
    _preferences.clear();
    Serial.println("Configuration cleared (factory reset)");
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
        Serial.println("ConfigManager not initialized");
        return;
    }
    
    _preferences.putString(PREF_WIFI_SSID, ssid);
    _preferences.putString(PREF_WIFI_PASS, password);
    Serial.println("WiFi credentials updated");
}

void ConfigManager::setImageURL(const String& url) {
    if (!_initialized && !begin()) {
        Serial.println("ConfigManager not initialized");
        return;
    }
    
    _preferences.putString(PREF_IMAGE_URL, url);
    Serial.println("Image URL updated: " + url);
}

void ConfigManager::setRefreshRate(int minutes) {
    if (!_initialized && !begin()) {
        Serial.println("ConfigManager not initialized");
        return;
    }
    
    if (minutes < 1) {
        Serial.println("Error: Refresh rate must be at least 1 minute");
        return;
    }
    
    _preferences.putInt(PREF_REFRESH_RATE, minutes);
    Serial.println("Refresh rate updated: " + String(minutes) + " minutes");
}

void ConfigManager::setMQTTConfig(const String& broker, const String& username, const String& password) {
    if (!_initialized && !begin()) {
        Serial.println("ConfigManager not initialized");
        return;
    }
    
    _preferences.putString(PREF_MQTT_BROKER, broker);
    _preferences.putString(PREF_MQTT_USER, username);
    _preferences.putString(PREF_MQTT_PASS, password);
    Serial.println("MQTT configuration updated");
}

void ConfigManager::setDebugMode(bool enabled) {
    if (!_initialized && !begin()) {
        Serial.println("ConfigManager not initialized");
        return;
    }

    _preferences.putBool(PREF_DEBUG_MODE, enabled);
    Serial.println("Debug mode updated: " + String(enabled ? "ON" : "OFF"));
}

void ConfigManager::setUseCRC32Check(bool enabled) {
    if (!_initialized && !begin()) {
        Serial.println("ConfigManager not initialized");
        return;
    }

    _preferences.putBool(PREF_USE_CRC32, enabled);
    Serial.println("CRC32 check updated: " + String(enabled ? "ON" : "OFF"));
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
        Serial.println("ConfigManager not initialized");
        return;
    }
    
    _preferences.putUInt(PREF_LAST_CRC32, crc32);
    Serial.println("Last CRC32 updated: 0x" + String(crc32, HEX));
}

void ConfigManager::markAsConfigured() {
    if (!_initialized && !begin()) {
        Serial.println("ConfigManager not initialized");
        return;
    }
    
    _preferences.putBool(PREF_CONFIGURED, true);
    Serial.println("Device marked as configured");
}
