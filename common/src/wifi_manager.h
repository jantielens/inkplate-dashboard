#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "config_manager.h"

// Access Point configuration
#define AP_SSID_PREFIX "inkplate-dashb-"
#define AP_TIMEOUT_MS 300000  // 5 minutes timeout for AP mode

// WiFi Client configuration
#define WIFI_CONNECT_TIMEOUT_MS 10000  // 10 seconds timeout for WiFi connection
#define WIFI_MAX_RETRIES 3

class WiFiManager {
public:
    WiFiManager(ConfigManager* configManager);
    ~WiFiManager();
    
    // Access Point Mode
    bool startAccessPoint();
    void stopAccessPoint();
    String getAPName();
    String getAPIPAddress();
    bool isAPActive();
    
    // WiFi Client Mode
    bool connectToWiFi(const String& ssid, const String& password);
    bool connectToWiFi();  // Uses stored credentials
    void disconnect();
    bool isConnected();
    String getLocalIP();
    int getRSSI();
    
    // Get WiFi status information
    String getStatusString();
    
private:
    ConfigManager* _configManager;
    String _apName;
    bool _apActive;
    
    // Generate unique device identifier for AP name
    String generateDeviceID();
};

#endif // WIFI_MANAGER_H
