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
        Logger::message("ConfigManager Error", "Failed to initialize Preferences");
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
        Logger::message("ConfigManager Error", "ConfigManager not initialized");
        return false;
    }
    
    // Load all configuration values
    config.isConfigured = _preferences.getBool(PREF_CONFIGURED, false);
    
    if (!config.isConfigured) {
        Logger::message("Config Status", "Device not configured yet");
        return false;
    }
    
    config.wifiSSID = _preferences.getString(PREF_WIFI_SSID, "");
    config.wifiPassword = _preferences.getString(PREF_WIFI_PASS, "");
    config.friendlyName = _preferences.getString(PREF_FRIENDLY_NAME, "");
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
    
    // Load static IP configuration (backwards compatible - defaults to DHCP if not set)
    config.useStaticIP = _preferences.getBool(PREF_USE_STATIC_IP, false);
    config.staticIP = _preferences.getString(PREF_STATIC_IP, "");
    config.gateway = _preferences.getString(PREF_GATEWAY, "");
    config.subnet = _preferences.getString(PREF_SUBNET, "");
    config.primaryDNS = _preferences.getString(PREF_PRIMARY_DNS, "");
    config.secondaryDNS = _preferences.getString(PREF_SECONDARY_DNS, "");
    
    // Load carousel configuration
    config.imageCount = _preferences.getUChar(PREF_IMAGE_COUNT, 0);
    if (config.imageCount > MAX_IMAGE_SLOTS) {
        config.imageCount = MAX_IMAGE_SLOTS;
    }
    
    for (uint8_t i = 0; i < config.imageCount; i++) {
        String urlKey = "img_url_" + String(i);
        String intKey = "img_int_" + String(i);
        String stayKey = String(PREF_IMAGE_STAY) + String(i);
        
        config.imageUrls[i] = _preferences.getString(urlKey.c_str(), "");
        config.imageIntervals[i] = _preferences.getInt(intKey.c_str(), DEFAULT_INTERVAL_MINUTES);
        config.imageStay[i] = _preferences.getBool(stayKey.c_str(), false);
    }
    
    // Load frontlight configuration (only for boards with HAS_FRONTLIGHT)
    config.frontlightDuration = _preferences.getUChar(PREF_FRONTLIGHT_DURATION, 0);  // Default: disabled
    config.frontlightBrightness = _preferences.getUChar(PREF_FRONTLIGHT_BRIGHTNESS, 63);  // Default: max brightness
    
    // Load overlay configuration
    config.overlayEnabled = _preferences.getBool(PREF_OVERLAY_ENABLED, false);
    config.overlayPosition = _preferences.getUChar(PREF_OVERLAY_POSITION, OVERLAY_POS_TOP_RIGHT);
    config.overlayShowBatteryIcon = _preferences.getBool(PREF_OVERLAY_SHOW_BATTERY_ICON, true);
    config.overlayShowBatteryPercentage = _preferences.getBool(PREF_OVERLAY_SHOW_BATTERY_PCT, true);
    config.overlayShowUpdateTime = _preferences.getBool(PREF_OVERLAY_SHOW_UPDATE_TIME, true);
    config.overlayShowCycleTime = _preferences.getBool(PREF_OVERLAY_SHOW_CYCLE_TIME, false);
    config.overlaySize = _preferences.getUChar(PREF_OVERLAY_SIZE, OVERLAY_SIZE_MEDIUM);
    config.overlayTextColor = _preferences.getUChar(PREF_OVERLAY_TEXT_COLOR, OVERLAY_COLOR_BLACK);
    
    // Validate configuration
    if (config.wifiSSID.length() == 0 || config.imageCount == 0) {
        Logger::message("Config Error", "Invalid configuration: missing SSID or images");
        return false;
    }
    
    Logger::begin("Configuration Loaded");
    Logger::line("SSID: " + config.wifiSSID);
    if (config.imageCount == 1) {
        Logger::linef("Single image, %dm interval", config.imageIntervals[0]);
        Logger::line("URL: " + config.imageUrls[0]);
    } else {
        Logger::linef("Carousel: %d images, avg %dm", config.imageCount, config.getAverageInterval());
    }
    if (config.mqttBroker.length() > 0) {
        Logger::linef("MQTT: %s (user: %s)", config.mqttBroker.c_str(), 
            config.mqttUsername.length() > 0 ? config.mqttUsername.c_str() : "none");
    }
    Logger::end();
    
    return true;
}

bool ConfigManager::saveConfig(const DashboardConfig& config) {
    if (!_initialized && !begin()) {
        Logger::message("ConfigManager Error", "ConfigManager not initialized");
        return false;
    }
    
    // Validate input
    if (config.wifiSSID.length() == 0) {
        Logger::message("Config Error", "WiFi SSID cannot be empty");
        return false;
    }
    
    if (config.imageCount == 0 || config.imageCount > MAX_IMAGE_SLOTS) {
        Logger::messagef("Config Error", "Invalid image count: %d (must be 1-%d)", config.imageCount, MAX_IMAGE_SLOTS);
        return false;
    }
    
    // Validate each image has URL and interval
    for (uint8_t i = 0; i < config.imageCount; i++) {
        if (config.imageUrls[i].length() == 0) {
            Logger::messagef("Config Error", "Image %d URL cannot be empty", i + 1);
            return false;
        }
        if (config.imageIntervals[i] < MIN_INTERVAL_MINUTES) {
            Logger::messagef("Config Error", "Image %d interval must be at least %d minute(s)", i + 1, MIN_INTERVAL_MINUTES);
            return false;
        }
    }
    
    // Save all configuration values
    _preferences.putString(PREF_WIFI_SSID, config.wifiSSID);
    _preferences.putString(PREF_WIFI_PASS, config.wifiPassword);
    _preferences.putString(PREF_FRIENDLY_NAME, config.friendlyName);
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
    
    // Save static IP configuration
    _preferences.putBool(PREF_USE_STATIC_IP, config.useStaticIP);
    _preferences.putString(PREF_STATIC_IP, config.staticIP);
    _preferences.putString(PREF_GATEWAY, config.gateway);
    _preferences.putString(PREF_SUBNET, config.subnet);
    _preferences.putString(PREF_PRIMARY_DNS, config.primaryDNS);
    _preferences.putString(PREF_SECONDARY_DNS, config.secondaryDNS);
    
    // Save config version
    _preferences.putUChar(PREF_CONFIG_VERSION, CONFIG_VERSION_CURRENT);
    
    // Save carousel configuration
    _preferences.putUChar(PREF_IMAGE_COUNT, config.imageCount);
    
    for (uint8_t i = 0; i < config.imageCount; i++) {
        String urlKey = "img_url_" + String(i);
        String intKey = "img_int_" + String(i);
        String stayKey = String(PREF_IMAGE_STAY) + String(i);
        
        size_t urlBytes = _preferences.putString(urlKey.c_str(), config.imageUrls[i]);
        if (urlBytes == 0) {
            Logger::messagef("Config Error", "Failed to save URL #%d", i);
            return false;
        }
        
        _preferences.putInt(intKey.c_str(), config.imageIntervals[i]);
        _preferences.putBool(stayKey.c_str(), config.imageStay[i]);
    }
    
    // Clear unused slots
    for (uint8_t i = config.imageCount; i < MAX_IMAGE_SLOTS; i++) {
        String urlKey = "img_url_" + String(i);
        String intKey = "img_int_" + String(i);
        String stayKey = String(PREF_IMAGE_STAY) + String(i);
        _preferences.remove(urlKey.c_str());
        _preferences.remove(intKey.c_str());
        _preferences.remove(stayKey.c_str());
    }
    
    // Save frontlight configuration (only for boards with HAS_FRONTLIGHT)
    _preferences.putUChar(PREF_FRONTLIGHT_DURATION, config.frontlightDuration);
    _preferences.putUChar(PREF_FRONTLIGHT_BRIGHTNESS, config.frontlightBrightness);
    
    // Save overlay configuration
    _preferences.putBool(PREF_OVERLAY_ENABLED, config.overlayEnabled);
    _preferences.putUChar(PREF_OVERLAY_POSITION, config.overlayPosition);
    _preferences.putBool(PREF_OVERLAY_SHOW_BATTERY_ICON, config.overlayShowBatteryIcon);
    _preferences.putBool(PREF_OVERLAY_SHOW_BATTERY_PCT, config.overlayShowBatteryPercentage);
    _preferences.putBool(PREF_OVERLAY_SHOW_UPDATE_TIME, config.overlayShowUpdateTime);
    _preferences.putBool(PREF_OVERLAY_SHOW_CYCLE_TIME, config.overlayShowCycleTime);
    _preferences.putUChar(PREF_OVERLAY_SIZE, config.overlaySize);
    _preferences.putUChar(PREF_OVERLAY_TEXT_COLOR, config.overlayTextColor);
    
    Logger::begin("Config Saved");
    if (config.imageCount == 1) {
        Logger::line("Single image mode");
    } else {
        Logger::linef("Carousel: %d images", config.imageCount);
    }
    Logger::end();
    
    return true;
}

void ConfigManager::clearConfig() {
    if (!_initialized && !begin()) {
        Logger::message("ConfigManager Error", "ConfigManager not initialized");
        return;
    }
    
    _preferences.clear();
    Logger::message("Factory Reset", "Configuration cleared (factory reset)");
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

String ConfigManager::getFriendlyName() {
    if (!_initialized && !begin()) {
        return "";
    }
    return _preferences.getString(PREF_FRIENDLY_NAME, "");
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
        Logger::message("ConfigManager Error", "ConfigManager not initialized");
        return;
    }
    
    _preferences.putString(PREF_WIFI_SSID, ssid);
    _preferences.putString(PREF_WIFI_PASS, password);
    Logger::message("Config Update", "WiFi credentials updated");
}

void ConfigManager::setFriendlyName(const String& name) {
    if (!_initialized && !begin()) {
        Logger::message("ConfigManager Error", "ConfigManager not initialized");
        return;
    }
    
    _preferences.putString(PREF_FRIENDLY_NAME, name);
    Logger::message("Config Update", "Friendly name updated");
}

void ConfigManager::setMQTTConfig(const String& broker, const String& username, const String& password) {
    if (!_initialized && !begin()) {
        Logger::message("ConfigManager Error", "ConfigManager not initialized");
        return;
    }
    
    _preferences.putString(PREF_MQTT_BROKER, broker);
    _preferences.putString(PREF_MQTT_USER, username);
    _preferences.putString(PREF_MQTT_PASS, password);
    Logger::message("Config Update", "MQTT configuration updated");
}

void ConfigManager::setDebugMode(bool enabled) {
    if (!_initialized && !begin()) {
        Logger::message("ConfigManager Error", "ConfigManager not initialized");
        return;
    }

    _preferences.putBool(PREF_DEBUG_MODE, enabled);
    Logger::begin("Config Update");
    Logger::line("Debug mode updated: " + String(enabled ? "ON" : "OFF"));
    Logger::end();
}

void ConfigManager::setUseCRC32Check(bool enabled) {
    if (!_initialized && !begin()) {
        Logger::message("ConfigManager Error", "ConfigManager not initialized");
        return;
    }

    _preferences.putBool(PREF_USE_CRC32, enabled);
    Logger::begin("Config Update");
    Logger::line("CRC32 check updated: " + String(enabled ? "ON" : "OFF"));
    Logger::end();
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
        Logger::message("ConfigManager Error", "ConfigManager not initialized");
        return;
    }
    
    // Validate rotation value (only 0, 1, 2, 3 are valid)
    if (rotation > 3) {
        Logger::message("ConfigManager Error", "Invalid rotation value: " + String(rotation) + " (must be 0-3)");
        return;
    }
    
    _preferences.putUChar(PREF_SCREEN_ROTATION, rotation);
    Logger::begin("Screen Rotation");
    Logger::line("Rotation updated: " + String(rotation * 90) + "°");
    Logger::end();
}

uint32_t ConfigManager::getLastCRC32() {
    if (!_initialized && !begin()) {
        return 0;
    }
    return _preferences.getUInt(PREF_LAST_CRC32, 0);
}

void ConfigManager::setLastCRC32(uint32_t crc32) {
    if (!_initialized && !begin()) {
        Logger::line("ConfigManager not initialized - cannot save CRC32");
        return;
    }
    
    _preferences.putUInt(PREF_LAST_CRC32, crc32);
    Logger::linef("Saved to preferences: 0x%08X", crc32);
}

void ConfigManager::markAsConfigured() {
    if (!_initialized && !begin()) {
        Logger::message("ConfigManager Error", "ConfigManager not initialized");
        return;
    }
    
    _preferences.putBool(PREF_CONFIGURED, true);
    Logger::message("Config Update", "Device marked as configured");
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
        Logger::message("ConfigManager Error", "ConfigManager not initialized");
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
        Logger::message("ConfigManager Error", "ConfigManager not initialized");
        return;
    }
    
    _preferences.putUChar(PREF_UPDATE_HOURS_0, hours[0]);
    _preferences.putUChar(PREF_UPDATE_HOURS_1, hours[1]);
    _preferences.putUChar(PREF_UPDATE_HOURS_2, hours[2]);
    
    Logger::messagef("Config Update", "Update hours bitmask set: 0x%02X%02X%02X", hours[2], hours[1], hours[0]);
}

int ConfigManager::getTimezoneOffset() {
    if (!_initialized && !begin()) {
        return 0;  // Default to UTC
    }
    return _preferences.getInt(PREF_TIMEZONE_OFFSET, 0);
}

void ConfigManager::setTimezoneOffset(int offset) {
    if (!_initialized && !begin()) {
        Logger::message("ConfigManager Error", "ConfigManager not initialized");
        return;
    }
    
    // Validate timezone offset (-12 to +14)
    if (offset < -12 || offset > 14) {
        Logger::messagef("Config Error", "Invalid timezone offset: %d (valid range: -12 to +14)", offset);
        return;
    }
    
    _preferences.putInt(PREF_TIMEZONE_OFFSET, offset);
    Logger::messagef("Config Update", "Timezone offset set to UTC%s%d", offset >= 0 ? "+" : "", offset);
}

// Static IP getters
bool ConfigManager::getUseStaticIP() {
    if (!_initialized && !begin()) {
        return false;  // Default to DHCP
    }
    return _preferences.getBool(PREF_USE_STATIC_IP, false);
}

String ConfigManager::getStaticIP() {
    if (!_initialized && !begin()) {
        return "";
    }
    return _preferences.getString(PREF_STATIC_IP, "");
}

String ConfigManager::getGateway() {
    if (!_initialized && !begin()) {
        return "";
    }
    return _preferences.getString(PREF_GATEWAY, "");
}

String ConfigManager::getSubnet() {
    if (!_initialized && !begin()) {
        return "";
    }
    return _preferences.getString(PREF_SUBNET, "");
}

String ConfigManager::getPrimaryDNS() {
    if (!_initialized && !begin()) {
        return "";
    }
    return _preferences.getString(PREF_PRIMARY_DNS, "");
}

String ConfigManager::getSecondaryDNS() {
    if (!_initialized && !begin()) {
        return "";
    }
    return _preferences.getString(PREF_SECONDARY_DNS, "");
}

// Static IP setter
void ConfigManager::setStaticIPConfig(bool useStatic, const String& ip, const String& gw, 
                                     const String& sn, const String& dns1, const String& dns2) {
    if (!_initialized && !begin()) {
        Logger::message("ConfigManager Error", "ConfigManager not initialized");
        return;
    }
    
    _preferences.putBool(PREF_USE_STATIC_IP, useStatic);
    _preferences.putString(PREF_STATIC_IP, ip);
    _preferences.putString(PREF_GATEWAY, gw);
    _preferences.putString(PREF_SUBNET, sn);
    _preferences.putString(PREF_PRIMARY_DNS, dns1);
    _preferences.putString(PREF_SECONDARY_DNS, dns2);
    
    if (useStatic) {
        Logger::begin("Static IP Config Saved");
        Logger::line("IP: " + ip);
        Logger::line("Gateway: " + gw);
        Logger::line("Subnet: " + sn);
        Logger::line("Primary DNS: " + dns1);
        if (dns2.length() > 0) {
            Logger::line("Secondary DNS: " + dns2);
        }
        Logger::end();
    } else {
        Logger::message("Config Update", "Network mode: DHCP");
    }
}

// WiFi channel locking methods
bool ConfigManager::hasWiFiChannelLock() {
    if (!_initialized && !begin()) {
        return false;
    }
    
    uint8_t channel = _preferences.getUChar(PREF_WIFI_CHANNEL, 0);
    return channel != 0;  // Channel 0 means no lock saved
}

uint8_t ConfigManager::getWiFiChannel() {
    if (!_initialized && !begin()) {
        return 0;
    }
    
    return _preferences.getUChar(PREF_WIFI_CHANNEL, 0);
}

void ConfigManager::getWiFiBSSID(uint8_t* bssid) {
    if (!_initialized && !begin()) {
        memset(bssid, 0, 6);
        return;
    }
    
    // Read 6 bytes from preferences
    size_t len = _preferences.getBytes(PREF_WIFI_BSSID, bssid, 6);
    
    if (len != 6) {
        // Invalid or missing BSSID, return zeros
        memset(bssid, 0, 6);
    }
}

void ConfigManager::setWiFiChannelLock(uint8_t channel, const uint8_t* bssid) {
    if (!_initialized && !begin()) {
        Logger::message("ConfigManager Error", "ConfigManager not initialized");
        return;
    }
    
    _preferences.putUChar(PREF_WIFI_CHANNEL, channel);
    _preferences.putBytes(PREF_WIFI_BSSID, bssid, 6);
    
    // Format BSSID for logging
    char bssidStr[18];
    snprintf(bssidStr, sizeof(bssidStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    
    Logger::begin("WiFi Channel Lock Saved");
    Logger::linef("Channel: %d", channel);
    Logger::line("BSSID: " + String(bssidStr));
    Logger::line("Fast reconnection enabled for next wake");
    Logger::end();
}

void ConfigManager::clearWiFiChannelLock() {
    if (!_initialized && !begin()) {
        return;
    }
    
    _preferences.putUChar(PREF_WIFI_CHANNEL, 0);
    // No need to clear BSSID, channel=0 indicates no lock
}

bool ConfigManager::sanitizeFriendlyName(const String& input, String& output) {
    output = "";
    
    if (input.length() == 0) {
        return true;  // Empty input is valid (means no friendly name)
    }
    
    // Convert to lowercase and filter valid characters
    for (unsigned int i = 0; i < input.length() && output.length() < 24; i++) {
        char c = input.charAt(i);
        
        // Convert uppercase to lowercase
        if (c >= 'A' && c <= 'Z') {
            c = c + ('a' - 'A');
        }
        
        // Only allow lowercase letters, digits, and hyphens
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-') {
            output += c;
        }
        // Skip any other characters (spaces, underscores, special chars)
    }
    
    // Remove leading hyphens
    while (output.length() > 0 && output.charAt(0) == '-') {
        output = output.substring(1);
    }
    
    // Remove trailing hyphens
    while (output.length() > 0 && output.charAt(output.length() - 1) == '-') {
        output = output.substring(0, output.length() - 1);
    }
    
    // If result is empty after sanitization, return false (invalid)
    if (output.length() == 0 && input.length() > 0) {
        return false;
    }
    
    return true;
}

