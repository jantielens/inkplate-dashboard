#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

// Configuration keys for Preferences storage
#define PREF_NAMESPACE "dashboard"
#define PREF_CONFIGURED "configured"
#define PREF_WIFI_SSID "wifi_ssid"
#define PREF_WIFI_PASS "wifi_pass"
#define PREF_IMAGE_URL "image_url"
#define PREF_REFRESH_RATE "refresh_rate"
#define PREF_MQTT_BROKER "mqtt_broker"
#define PREF_MQTT_USER "mqtt_user"
#define PREF_MQTT_PASS "mqtt_pass"
#define PREF_DEBUG_MODE "debug_mode"

// Default values
#define DEFAULT_REFRESH_RATE 5  // 5 minutes

// Configuration data structure
struct DashboardConfig {
    String wifiSSID;
    String wifiPassword;
    String imageURL;
    int refreshRate;  // in minutes
    String mqttBroker;  // MQTT broker URL (e.g., mqtt://broker.example.com:1883)
    String mqttUsername;
    String mqttPassword;
    bool isConfigured;
    bool debugMode;
    
    // Constructor with defaults
    DashboardConfig() : 
        wifiSSID(""),
        wifiPassword(""),
        imageURL(""),
        refreshRate(DEFAULT_REFRESH_RATE),
        mqttBroker(""),
        mqttUsername(""),
        mqttPassword(""),
        isConfigured(false),
        debugMode(false) {}
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
    String getImageURL();
    int getRefreshRate();
    String getMQTTBroker();
    String getMQTTUsername();
    String getMQTTPassword();
    bool getDebugMode();
    
    // Individual setters
    void setWiFiCredentials(const String& ssid, const String& password);
    void setImageURL(const String& url);
    void setRefreshRate(int minutes);
    void setMQTTConfig(const String& broker, const String& username, const String& password);
    void setDebugMode(bool enabled);
    
    // Mark device as configured
    void markAsConfigured();
    
private:
    Preferences _preferences;
    bool _initialized;
};

#endif // CONFIG_MANAGER_H
