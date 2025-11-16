#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "config_manager.h"
#include "power_manager.h"

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
    bool connectToWiFi(const String& ssid, const String& password, uint8_t* outRetryCount = nullptr, bool disableAutoReconnect = false);
    bool connectToWiFi(uint8_t* outRetryCount = nullptr, bool disableAutoReconnect = false);  // Uses stored credentials
    void disconnect();
    bool isConnected();
    String getLocalIP();
    int getRSSI();
    
    // Get WiFi status information
    String getStatusString();
    
    // Static IP configuration
    bool configureStaticIP(const String& ip, const String& gateway, const String& subnet, 
                          const String& dns1, const String& dns2 = "");
    
    // Power management integration
    void setPowerManager(PowerManager* powerManager);
    
    // Device identification
    String generateDeviceID();  // Generate MAC-based ID (e.g., "AABBCC")
    String getDeviceIdentifier();  // Get friendly name if set, else "inkplate-XXXXXX"
    
    // mDNS support
    bool startMDNS();           // Start mDNS service
    void stopMDNS();            // Stop mDNS service
    String getMDNSHostname();   // Get the .local hostname (empty if mDNS not active)
    
private:
    ConfigManager* _configManager;
    PowerManager* _powerManager;
    String _apName;
    bool _apActive;
    bool _mdnsActive;
};

#endif // WIFI_MANAGER_H
