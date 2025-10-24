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
        return false;
    }
    
    // Initialize config version if not set (new device)
    uint8_t configVersion = _preferences.getUChar(PREF_CONFIG_VERSION, 0);
    if (configVersion == 0) {
        _preferences.putUChar(PREF_CONFIG_VERSION, CONFIG_VERSION_CURRENT);
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
    uint8_t imageCount = _preferences.getUChar(PREF_IMAGE_COUNT, 0);
    
    return (ssid.length() > 0 && imageCount > 0);
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
    config.mqttBroker = _preferences.getString(PREF_MQTT_BROKER, "");
    config.mqttUsername = _preferences.getString(PREF_MQTT_USER, "");
    config.mqttPassword = _preferences.getString(PREF_MQTT_PASS, "");
    config.debugMode = _preferences.getBool(PREF_DEBUG_MODE, false);
    config.useCRC32Check = _preferences.getBool(PREF_USE_CRC32, false);
    
    // Load hourly schedule (3 bytes for 24-bit bitmask)
    config.updateHours[0] = _preferences.getUChar(PREF_UPDATE_HOURS_0, 0xFF);
    config.updateHours[1] = _preferences.getUChar(PREF_UPDATE_HOURS_1, 0xFF);
    config.updateHours[2] = _preferences.getUChar(PREF_UPDATE_HOURS_2, 0xFF);
    
    // Load timezone offset
    config.timezoneOffset = _preferences.getInt(PREF_TIMEZONE_OFFSET, 0);
    
    // Load screen rotation
    config.screenRotation = _preferences.getUChar(PREF_SCREEN_ROTATION, DEFAULT_SCREEN_ROTATION);
    
    // Load carousel configuration
    config.imageCount = _preferences.getUChar(PREF_IMAGE_COUNT, 0);
    if (config.imageCount > MAX_IMAGE_SLOTS) {
        config.imageCount = MAX_IMAGE_SLOTS;
    }
    
    for (uint8_t i = 0; i < config.imageCount; i++) {
        String urlKey = "img_url_" + String(i);
        String intKey = "img_int_" + String(i);
        
        config.imageUrls[i] = _preferences.getString(urlKey.c_str(), "");
        config.imageIntervals[i] = _preferences.getInt(intKey.c_str(), DEFAULT_INTERVAL_MINUTES);
    }
    
    // Validate configuration
    if (config.wifiSSID.length() == 0 || config.imageCount == 0) {
        LogBox::begin("Config Error");
        LogBox::line("Invalid configuration: missing SSID or images");
        LogBox::end();
        return false;
    }
    
    LogBox::begin("Configuration Loaded");
    LogBox::line("WiFi SSID: " + config.wifiSSID);
    if (config.imageCount == 1) {
        LogBox::line("Mode: Single Image");
        LogBox::line("Image URL: " + config.imageUrls[0]);
        LogBox::linef("Interval: %d minutes", config.imageIntervals[0]);
    } else {
        LogBox::linef("Mode: Carousel (%d images)", config.imageCount);
        LogBox::linef("Average Interval: %d minutes", config.getAverageInterval());
    }
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
    
    if (config.imageCount == 0 || config.imageCount > MAX_IMAGE_SLOTS) {
        LogBox::begin("Config Error");
        LogBox::linef("Invalid image count: %d (must be 1-%d)", config.imageCount, MAX_IMAGE_SLOTS);
        LogBox::end();
        return false;
    }
    
    // Validate each image has URL and interval
    for (uint8_t i = 0; i < config.imageCount; i++) {
        if (config.imageUrls[i].length() == 0) {
            LogBox::begin("Config Error");
            LogBox::linef("Image %d URL cannot be empty", i + 1);
            LogBox::end();
            return false;
        }
        if (config.imageIntervals[i] < MIN_INTERVAL_MINUTES) {
            LogBox::begin("Config Error");
            LogBox::linef("Image %d interval must be at least %d minute(s)", i + 1, MIN_INTERVAL_MINUTES);
            LogBox::end();
            return false;
        }
    }
    
    // Save all configuration values
    _preferences.putString(PREF_WIFI_SSID, config.wifiSSID);
    _preferences.putString(PREF_WIFI_PASS, config.wifiPassword);
    _preferences.putString(PREF_MQTT_BROKER, config.mqttBroker);
    _preferences.putString(PREF_MQTT_USER, config.mqttUsername);
    _preferences.putString(PREF_MQTT_PASS, config.mqttPassword);
    _preferences.putBool(PREF_CONFIGURED, true);
    _preferences.putBool(PREF_DEBUG_MODE, config.debugMode);
    _preferences.putBool(PREF_USE_CRC32, config.useCRC32Check);
    
    // Save hourly schedule (3 bytes for 24-bit bitmask)
    _preferences.putUChar(PREF_UPDATE_HOURS_0, config.updateHours[0]);
    _preferences.putUChar(PREF_UPDATE_HOURS_1, config.updateHours[1]);
    _preferences.putUChar(PREF_UPDATE_HOURS_2, config.updateHours[2]);
    
    // Save timezone offset
    _preferences.putInt(PREF_TIMEZONE_OFFSET, config.timezoneOffset);
    
    // Save screen rotation
    _preferences.putUChar(PREF_SCREEN_ROTATION, config.screenRotation);
    
    // Save config version
    _preferences.putUChar(PREF_CONFIG_VERSION, CONFIG_VERSION_CURRENT);
    
    // Save carousel configuration
    _preferences.putUChar(PREF_IMAGE_COUNT, config.imageCount);
    
    for (uint8_t i = 0; i < config.imageCount; i++) {
        String urlKey = "img_url_" + String(i);
        String intKey = "img_int_" + String(i);
        
        size_t urlBytes = _preferences.putString(urlKey.c_str(), config.imageUrls[i]);
        if (urlBytes == 0) {
            LogBox::begin("Config Error");
            LogBox::linef("Failed to save URL #%d", i);
            LogBox::end();
            return false;
        }
        
        _preferences.putInt(intKey.c_str(), config.imageIntervals[i]);
    }
    
    // Clear unused slots
    for (uint8_t i = config.imageCount; i < MAX_IMAGE_SLOTS; i++) {
        String urlKey = "img_url_" + String(i);
        String intKey = "img_int_" + String(i);
        _preferences.remove(urlKey.c_str());
        _preferences.remove(intKey.c_str());
    }
    
    LogBox::begin("Config Saved");
    LogBox::line("Configuration saved successfully");
    if (config.imageCount == 1) {
        LogBox::line("Mode: Single Image");
    } else {
        LogBox::linef("Mode: Carousel (%d images)", config.imageCount);
    }
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

uint8_t ConfigManager::getScreenRotation() {
    if (!_initialized && !begin()) {
        return DEFAULT_SCREEN_ROTATION;
    }
    return _preferences.getUChar(PREF_SCREEN_ROTATION, DEFAULT_SCREEN_ROTATION);
}

void ConfigManager::setScreenRotation(uint8_t rotation) {
    if (!_initialized && !begin()) {
        LogBox::begin("ConfigManager Error");
        LogBox::line("ConfigManager not initialized");
        LogBox::end();
        return;
    }
    
    // Validate rotation value (only 0, 1, 2, 3 are valid)
    if (rotation > 3) {
        LogBox::begin("ConfigManager Error");
        LogBox::line("Invalid rotation value: " + String(rotation) + " (must be 0-3)");
        LogBox::end();
        return;
    }
    
    _preferences.putUChar(PREF_SCREEN_ROTATION, rotation);
    LogBox::begin("Screen Rotation");
    LogBox::line("Rotation updated: " + String(rotation * 90) + "°");
    LogBox::end();
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

bool ConfigManager::isHourEnabled(uint8_t hour) {
    if (hour > 23) {
        return false;
    }
    
    // Determine which byte and bit position
    uint8_t byteIndex = hour / 8;      // 0, 1, or 2
    uint8_t bitPosition = hour % 8;    // 0-7 within the byte
    
    if (!_initialized && !begin()) {
        // Default: all hours enabled
        return true;
    }
    
    uint8_t hourByte = _preferences.getUChar(PREF_UPDATE_HOURS_0 + byteIndex, 0xFF);
    return (hourByte >> bitPosition) & 1;
}

void ConfigManager::setHourEnabled(uint8_t hour, bool enabled) {
    if (hour > 23) {
        return;
    }
    
    if (!_initialized && !begin()) {
        LogBox::begin("ConfigManager Error");
        LogBox::line("ConfigManager not initialized");
        LogBox::end();
        return;
    }
    
    uint8_t byteIndex = hour / 8;
    uint8_t bitPosition = hour % 8;
    
    // Read current byte
    uint8_t hourByte = _preferences.getUChar(PREF_UPDATE_HOURS_0 + byteIndex, 0xFF);
    
    // Modify bit
    if (enabled) {
        hourByte |= (1 << bitPosition);
    } else {
        hourByte &= ~(1 << bitPosition);
    }
    
    // Save byte
    _preferences.putUChar(PREF_UPDATE_HOURS_0 + byteIndex, hourByte);
}

void ConfigManager::getUpdateHours(uint8_t hours[3]) {
    if (!_initialized && !begin()) {
        // Default: all hours enabled
        hours[0] = 0xFF;
        hours[1] = 0xFF;
        hours[2] = 0xFF;
        return;
    }
    
    hours[0] = _preferences.getUChar(PREF_UPDATE_HOURS_0, 0xFF);
    hours[1] = _preferences.getUChar(PREF_UPDATE_HOURS_1, 0xFF);
    hours[2] = _preferences.getUChar(PREF_UPDATE_HOURS_2, 0xFF);
}

void ConfigManager::setUpdateHours(const uint8_t hours[3]) {
    if (!_initialized && !begin()) {
        LogBox::begin("ConfigManager Error");
        LogBox::line("ConfigManager not initialized");
        LogBox::end();
        return;
    }
    
    _preferences.putUChar(PREF_UPDATE_HOURS_0, hours[0]);
    _preferences.putUChar(PREF_UPDATE_HOURS_1, hours[1]);
    _preferences.putUChar(PREF_UPDATE_HOURS_2, hours[2]);
    
    LogBox::begin("Config Update");
    LogBox::linef("Update hours bitmask set: 0x%02X%02X%02X", hours[2], hours[1], hours[0]);
    LogBox::end();
}

int ConfigManager::getTimezoneOffset() {
    if (!_initialized && !begin()) {
        return 0;  // Default to UTC
    }
    return _preferences.getInt(PREF_TIMEZONE_OFFSET, 0);
}

void ConfigManager::setTimezoneOffset(int offset) {
    if (!_initialized && !begin()) {
        LogBox::begin("ConfigManager Error");
        LogBox::line("ConfigManager not initialized");
        LogBox::end();
        return;
    }
    
    // Validate timezone offset (-12 to +14)
    if (offset < -12 || offset > 14) {
        LogBox::begin("Config Error");
        LogBox::linef("Invalid timezone offset: %d (valid range: -12 to +14)", offset);
        LogBox::end();
        return;
    }
    
    _preferences.putInt(PREF_TIMEZONE_OFFSET, offset);
    LogBox::begin("Config Update");
    LogBox::linef("Timezone offset set to UTC%s%d", offset >= 0 ? "+" : "", offset);
    LogBox::end();
}

