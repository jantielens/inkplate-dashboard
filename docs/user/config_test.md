# ConfigManager Usage Example

This document shows how to use the ConfigManager class to store and retrieve configuration.

## Basic Usage

```cpp
#include <src/config_manager.h>

ConfigManager configManager;

void setup() {
    Serial.begin(115200);
    
    // Initialize the config manager
    if (!configManager.begin()) {
        Serial.println("Failed to initialize ConfigManager!");
        return;
    }
    
    // Check if device is configured
    if (configManager.isConfigured()) {
        Serial.println("Device is already configured");
        
        // Load existing configuration
        DashboardConfig config;
        if (configManager.loadConfig(config)) {
            Serial.println("WiFi SSID: " + config.wifiSSID);
            Serial.println("Image URL: " + config.imageURL);
            Serial.println("Refresh Rate: " + String(config.refreshRate));
        }
    } else {
        Serial.println("First boot - device not configured");
        
        // Create and save new configuration
        DashboardConfig config;
        config.wifiSSID = "MyWiFi";
        config.wifiPassword = "MyPassword";
        config.imageURL = "https://example.com/image.png";
        config.refreshRate = 10;  // 10 minutes
        
        if (configManager.saveConfig(config)) {
            Serial.println("Configuration saved!");
        }
    }
}
```

## Individual Setters/Getters

```cpp
// Update individual settings
configManager.setWiFiCredentials("NewSSID", "NewPassword");
configManager.setImageURL("https://example.com/new-image.png");
configManager.setRefreshRate(15);

// Get individual settings
String ssid = configManager.getWiFiSSID();
String url = configManager.getImageURL();
int rate = configManager.getRefreshRate();
```

## Factory Reset

```cpp
// Clear all configuration
configManager.clearConfig();
Serial.println("Factory reset complete");
```

## Configuration Data Structure

```cpp
struct DashboardConfig {
    String wifiSSID;        // WiFi network name
    String wifiPassword;    // WiFi password
    String imageURL;        // URL of image to download
    int refreshRate;        // Refresh interval in minutes
    bool isConfigured;      // Whether device has been configured
};
```

## Default Values

- **Refresh Rate**: 5 minutes (defined as `DEFAULT_REFRESH_RATE`)
- **Configured Status**: false (first boot)

## Validation

The `saveConfig()` method validates:
- WiFi SSID must not be empty
- Image URL must not be empty
- Refresh rate must be at least 1 minute

## Storage

All configuration is stored in ESP32's NVS (Non-Volatile Storage) using the Preferences library with namespace "dashboard".
