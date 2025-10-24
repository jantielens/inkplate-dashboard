#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

// Configuration keys for Preferences storage
#define PREF_NAMESPACE "dashboard"
#define PREF_CONFIGURED "configured"
#define PREF_WIFI_SSID "wifi_ssid"
#define PREF_WIFI_PASS "wifi_pass"
#define PREF_IMAGE_URL "image_url"  // Legacy - kept for compatibility
#define PREF_REFRESH_RATE "refresh_rate"  // Legacy - kept for compatibility
#define PREF_MQTT_BROKER "mqtt_broker"
#define PREF_MQTT_USER "mqtt_user"
#define PREF_MQTT_PASS "mqtt_pass"
#define PREF_DEBUG_MODE "debug_mode"
#define PREF_USE_CRC32 "use_crc32"
#define PREF_LAST_CRC32 "last_crc32"
#define PREF_UPDATE_HOURS_0 "upd_hours_0"
#define PREF_UPDATE_HOURS_1 "upd_hours_1"
#define PREF_UPDATE_HOURS_2 "upd_hours_2"
#define PREF_TIMEZONE_OFFSET "tz_offset"
#define PREF_SCREEN_ROTATION "screen_rot"

// Carousel configuration keys
#define PREF_CONFIG_VERSION "cfg_ver"
#define PREF_IMAGE_COUNT "img_count"
#define CONFIG_VERSION_CURRENT 2

// Carousel constraints
#define MAX_IMAGE_SLOTS 10
#define MAX_URL_LENGTH 250
#define MIN_INTERVAL_MINUTES 1
#define DEFAULT_INTERVAL_MINUTES 5

// Default values
#define DEFAULT_SCREEN_ROTATION 0  // 0 degrees (landscape)

// Configuration data structure
struct DashboardConfig {
    String wifiSSID;
    String wifiPassword;
    String mqttBroker;  // MQTT broker URL (e.g., mqtt://broker.example.com:1883)
    String mqttUsername;
    String mqttPassword;
    bool isConfigured;
    bool debugMode;
    bool useCRC32Check;  // Enable CRC32-based change detection
    uint8_t updateHours[3];  // 24-bit bitmask: bit i = hour i enabled (0-23)
    int timezoneOffset;  // Timezone offset in hours (-12 to +14)
    uint8_t screenRotation;  // Screen rotation: 0, 1, 2, 3 (0°, 90°, 180°, 270°)
    
    // Carousel configuration
    uint8_t imageCount;           // How many URLs provided (0-10)
    String imageUrls[MAX_IMAGE_SLOTS];    // Image URLs
    int imageIntervals[MAX_IMAGE_SLOTS];  // Display duration per image in minutes
    
    // Constructor with defaults
    DashboardConfig() : 
        wifiSSID(""),
        wifiPassword(""),
        mqttBroker(""),
        mqttUsername(""),
        mqttPassword(""),
        isConfigured(false),
        debugMode(false),
        useCRC32Check(false),
        timezoneOffset(0),
        screenRotation(DEFAULT_SCREEN_ROTATION),
        imageCount(0) {
        // Initialize all hours enabled by default (0xFF = all bits set)
        updateHours[0] = 0xFF;  // Hours 0-7
        updateHours[1] = 0xFF;  // Hours 8-15
        updateHours[2] = 0xFF;  // Hours 16-23
        
        // Initialize carousel arrays
        for (int i = 0; i < MAX_IMAGE_SLOTS; i++) {
            imageUrls[i] = "";
            imageIntervals[i] = 0;
        }
    }
    
    // Helper methods
    bool isCarouselMode() const {
        return imageCount > 1;
    }
    
    // Calculate average interval for battery estimates and fallbacks
    int getAverageInterval() const {
        if (imageCount == 0) return DEFAULT_INTERVAL_MINUTES;
        int total = 0;
        for (uint8_t i = 0; i < imageCount; i++) {
            total += imageIntervals[i];
        }
        return total / imageCount;
    }
};

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();
    
    // Initialize the configuration manager
    bool begin();
    
    // Check if device has been configured
    bool isConfigured();
    
    // Check if WiFi credentials are configured
    bool hasWiFiConfig();
    
    // Check if device is fully configured (WiFi + Image URL)
    bool isFullyConfigured();
    
    // Load configuration from storage
    bool loadConfig(DashboardConfig& config);
    
    // Save configuration to storage
    bool saveConfig(const DashboardConfig& config);
    
    // Clear all configuration (factory reset)
    void clearConfig();
    
    // Individual getters
    String getWiFiSSID();
    String getWiFiPassword();
    String getMQTTBroker();
    String getMQTTUsername();
    String getMQTTPassword();
    bool getDebugMode();
    bool getUseCRC32Check();
    uint8_t getScreenRotation();
    
    // Individual setters
    void setWiFiCredentials(const String& ssid, const String& password);
    void setMQTTConfig(const String& broker, const String& username, const String& password);
    void setDebugMode(bool enabled);
    void setUseCRC32Check(bool enabled);
    void setScreenRotation(uint8_t rotation);
    
    // CRC32 storage management
    uint32_t getLastCRC32();
    void setLastCRC32(uint32_t crc32);
    
    // Hourly scheduling (24-bit bitmask)
    bool isHourEnabled(uint8_t hour);  // hour: 0-23, returns true if updates allowed
    void setHourEnabled(uint8_t hour, bool enabled);  // hour: 0-23
    void getUpdateHours(uint8_t hours[3]);  // Get entire bitmask
    void setUpdateHours(const uint8_t hours[3]);  // Set entire bitmask
    
    // Timezone offset
    int getTimezoneOffset();
    void setTimezoneOffset(int offset);  // offset: -12 to +14 hours
    
    // Mark device as configured
    void markAsConfigured();
    
    // === Static Helper Methods for Hourly Scheduling ===
    /**
     * Check if a specific hour is enabled in the update schedule bitmask
     * @param hour Hour number (0-23)
     * @param updateHours Bitmask array (3 bytes for 24 hours)
     * @return true if hour is enabled for updates, false otherwise
     */
    static bool isHourEnabledInBitmask(uint8_t hour, const uint8_t updateHours[3]) {
        if (hour > 23) return false;
        uint8_t byteIndex = hour / 8;
        uint8_t bitPosition = hour % 8;
        return (updateHours[byteIndex] >> bitPosition) & 1;
    }
    
    /**
     * Apply timezone offset to UTC hour with proper wrapping
     * Handles negative offsets and day boundary wrapping correctly
     * @param utcHour UTC hour (0-23)
     * @param timezoneOffset Timezone offset in hours (-12 to +14)
     * @return Local hour (0-23) adjusted for timezone
     */
    static int applyTimezoneOffset(int utcHour, int timezoneOffset) {
        int localHour = (utcHour + timezoneOffset) % 24;
        if (localHour < 0) {
            localHour += 24;
        }
        return localHour;
    }
    
private:
    Preferences _preferences;
    bool _initialized;
};

#endif // CONFIG_MANAGER_H
