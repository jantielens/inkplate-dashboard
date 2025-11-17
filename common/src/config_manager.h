#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "config_logic.h"

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

// Static IP configuration keys
#define PREF_USE_STATIC_IP "use_static_ip"
#define PREF_STATIC_IP "static_ip"
#define PREF_GATEWAY "gateway"
#define PREF_SUBNET "subnet"
#define PREF_PRIMARY_DNS "dns1"
#define PREF_SECONDARY_DNS "dns2"

// WiFi channel locking keys (for fast reconnection)
#define PREF_WIFI_CHANNEL "wifi_ch"
#define PREF_WIFI_BSSID "wifi_bssid"

// Device identification
#define PREF_FRIENDLY_NAME "friendly_name"

// Frontlight configuration (Inkplate 6 Flick only)
#define PREF_FRONTLIGHT_DURATION "fl_duration"  // Duration in seconds (0 = disabled)
#define PREF_FRONTLIGHT_BRIGHTNESS "fl_bright"  // Brightness level (0-63)

// Overlay configuration keys
#define PREF_OVERLAY_ENABLED "ovl_enabled"
#define PREF_OVERLAY_POSITION "ovl_pos"
#define PREF_OVERLAY_SHOW_BATTERY_ICON "ovl_bat_icon"
#define PREF_OVERLAY_SHOW_BATTERY_PCT "ovl_bat_pct"
#define PREF_OVERLAY_SHOW_UPDATE_TIME "ovl_upd_time"
#define PREF_OVERLAY_SHOW_CYCLE_TIME "ovl_cyc_time"
#define PREF_OVERLAY_SIZE "ovl_size"
#define PREF_OVERLAY_TEXT_COLOR "ovl_txt_col"

// Carousel configuration keys
#define PREF_CONFIG_VERSION "cfg_ver"
#define PREF_IMAGE_COUNT "img_count"
#define PREF_IMAGE_STAY "img_stay_"  // Followed by index 0-9
#define CONFIG_VERSION_CURRENT 2

// Carousel constraints
#define MAX_IMAGE_SLOTS 10
#define MAX_URL_LENGTH 250
#define MIN_INTERVAL_MINUTES 0  // 0 = button-only mode (no automatic refresh)
#define DEFAULT_INTERVAL_MINUTES 5

// Default values
#define DEFAULT_SCREEN_ROTATION 0  // 0 degrees (landscape)

// Overlay position enum (matches config)
#define OVERLAY_POS_TOP_LEFT 0
#define OVERLAY_POS_TOP_RIGHT 1
#define OVERLAY_POS_BOTTOM_LEFT 2
#define OVERLAY_POS_BOTTOM_RIGHT 3

// Overlay size enum (matches config)
#define OVERLAY_SIZE_SMALL 0
#define OVERLAY_SIZE_MEDIUM 1
#define OVERLAY_SIZE_LARGE 2

// Overlay text color enum (matches config)
#define OVERLAY_COLOR_BLACK 0
#define OVERLAY_COLOR_WHITE 1

// Configuration data structure
struct DashboardConfig {
    String wifiSSID;
    String wifiPassword;
    String friendlyName;  // User-friendly device name (optional, for MQTT/HA/hostname)
    String mqttBroker;  // MQTT broker URL (e.g., mqtt://broker.example.com:1883)
    String mqttUsername;
    String mqttPassword;
    bool isConfigured;
    bool debugMode;
    bool useCRC32Check;  // Enable CRC32-based change detection
    uint8_t updateHours[3];  // 24-bit bitmask: bit i = hour i enabled (0-23)
    int timezoneOffset;  // Timezone offset in hours (-12 to +14)
    uint8_t screenRotation;  // Screen rotation: 0, 1, 2, 3 (0°, 90°, 180°, 270°)
    
    // Static IP configuration
    bool useStaticIP;       // Use static IP instead of DHCP
    String staticIP;        // Static IP address (e.g., "192.168.1.100")
    String gateway;         // Gateway address (e.g., "192.168.1.1")
    String subnet;          // Subnet mask (e.g., "255.255.255.0")
    String primaryDNS;      // Primary DNS server (e.g., "8.8.8.8")
    String secondaryDNS;    // Secondary DNS server (optional)
    
    // Carousel configuration
    uint8_t imageCount;           // How many URLs provided (0-10)
    String imageUrls[MAX_IMAGE_SLOTS];    // Image URLs
    int imageIntervals[MAX_IMAGE_SLOTS];  // Display duration per image in minutes
    bool imageStay[MAX_IMAGE_SLOTS];      // Stay on image (don't auto-advance)
    
    // Frontlight configuration (only for boards with HAS_FRONTLIGHT)
    uint8_t frontlightDuration;   // Duration in seconds (0 = disabled, default 0)
    uint8_t frontlightBrightness; // Brightness level (0-63, default 63)
    
    // Overlay configuration (on-image status display)
    bool overlayEnabled;               // Enable/disable overlay
    uint8_t overlayPosition;           // 0=TL, 1=TR, 2=BL, 3=BR
    bool overlayShowBatteryIcon;       // Show battery icon
    bool overlayShowBatteryPercentage; // Show battery percentage text
    bool overlayShowUpdateTime;        // Show last update time
    bool overlayShowCycleTime;         // Show last cycle/loop time
    uint8_t overlaySize;               // 0=Small, 1=Medium, 2=Large
    uint8_t overlayTextColor;          // 0=Black, 1=White
    
    // Constructor with defaults
    DashboardConfig() : 
        wifiSSID(""),
        wifiPassword(""),
        friendlyName(""),
        mqttBroker(""),
        mqttUsername(""),
        mqttPassword(""),
        isConfigured(false),
        debugMode(false),
        useCRC32Check(false),
        timezoneOffset(0),
        screenRotation(DEFAULT_SCREEN_ROTATION),
        useStaticIP(false),
        staticIP(""),
        gateway(""),
        subnet(""),
        primaryDNS(""),
        secondaryDNS(""),
        imageCount(0),
        frontlightDuration(0),      // Default: disabled
        frontlightBrightness(63),   // Default: max brightness
        overlayEnabled(false),      // Default: disabled
        overlayPosition(OVERLAY_POS_TOP_RIGHT),
        overlayShowBatteryIcon(true),
        overlayShowBatteryPercentage(true),
        overlayShowUpdateTime(true),
        overlayShowCycleTime(false),
        overlaySize(OVERLAY_SIZE_MEDIUM),
        overlayTextColor(OVERLAY_COLOR_BLACK) {
        // Initialize all hours enabled by default (0xFF = all bits set)
        updateHours[0] = 0xFF;  // Hours 0-7
        updateHours[1] = 0xFF;  // Hours 8-15
        updateHours[2] = 0xFF;  // Hours 16-23
        
        // Initialize carousel arrays
        for (int i = 0; i < MAX_IMAGE_SLOTS; i++) {
            imageUrls[i] = "";
            imageIntervals[i] = 0;
            imageStay[i] = false;
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
    String getFriendlyName();
    String getMQTTBroker();
    String getMQTTUsername();
    String getMQTTPassword();
    bool getDebugMode();
    bool getUseCRC32Check();
    uint8_t getScreenRotation();
    
    // Static IP getters
    bool getUseStaticIP();
    String getStaticIP();
    String getGateway();
    String getSubnet();
    String getPrimaryDNS();
    String getSecondaryDNS();
    
    // Individual setters
    void setWiFiCredentials(const String& ssid, const String& password);
    void setFriendlyName(const String& name);
    void setMQTTConfig(const String& broker, const String& username, const String& password);
    void setDebugMode(bool enabled);
    void setUseCRC32Check(bool enabled);
    void setScreenRotation(uint8_t rotation);
    
    // Static IP setters
    void setStaticIPConfig(bool useStatic, const String& ip, const String& gw, 
                          const String& sn, const String& dns1, const String& dns2);
    
    // WiFi channel locking (for fast reconnection)
    bool hasWiFiChannelLock();
    uint8_t getWiFiChannel();
    void getWiFiBSSID(uint8_t* bssid);  // Copies 6 bytes to provided array
    void setWiFiChannelLock(uint8_t channel, const uint8_t* bssid);
    void clearWiFiChannelLock();
    
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
    
    // Friendly name validation and sanitization
    // Returns true if sanitization successful, false if input completely invalid
    // Rules: lowercase a-z, digits 0-9, hyphens; max 24 chars; no leading/trailing hyphens
    static bool sanitizeFriendlyName(const String& input, String& output);
    
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
        // Delegate to standalone function for testability
        return ::isHourEnabledInBitmask(hour, updateHours);
    }
    
    /**
     * Apply timezone offset to UTC hour with proper wrapping
     * Handles negative offsets and day boundary wrapping correctly
     * @param utcHour UTC hour (0-23)
     * @param timezoneOffset Timezone offset in hours (-12 to +14)
     * @return Local hour (0-23) adjusted for timezone
     */
    static int applyTimezoneOffset(int utcHour, int timezoneOffset) {
        // Delegate to standalone function for testability
        return ::applyTimezoneOffset(utcHour, timezoneOffset);
    }
    
    /**
     * Check if all 24 hours are enabled in the update schedule bitmask
     * @param updateHours Bitmask array (3 bytes for 24 hours)
     * @return true if all 24 hours are enabled, false otherwise
     */
    static bool areAllHoursEnabled(const uint8_t updateHours[3]) {
        // Delegate to standalone function for testability
        return ::areAllHoursEnabled(updateHours);
    }
    
private:
    Preferences _preferences;
    bool _initialized;
};

#endif // CONFIG_MANAGER_H
