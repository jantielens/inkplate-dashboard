#ifndef CONFIG_PORTAL_H
#define CONFIG_PORTAL_H

#include <Arduino.h>
#include <WebServer.h>
#include "config_manager.h"
#include "wifi_manager.h"

// Portal mode enum
enum PortalMode {
    BOOT_MODE,      // First boot - only WiFi credentials
    CONFIG_MODE     // Configuration mode - all settings including factory reset
};

class ConfigPortal {
public:
    ConfigPortal(ConfigManager* configManager, WiFiManager* wifiManager);
    ~ConfigPortal();
    
    // Start the configuration web server with specified mode
    bool begin(PortalMode mode = CONFIG_MODE);
    
    // Stop the configuration web server
    void stop();
    
    // Handle client requests (call in loop)
    void handleClient();
    
    // Check if configuration was submitted
    bool isConfigReceived();
    
    // Get the port number
    int getPort();
    
private:
    ConfigManager* _configManager;
    WiFiManager* _wifiManager;
    WebServer* _server;
    bool _configReceived;
    int _port;
    PortalMode _mode;
    
    // HTTP handlers
    void handleRoot();
    void handleSubmit();
    void handleFactoryReset();
    void handleNotFound();
    
    // HTML page generators
    String generateConfigPage();
    String generateSuccessPage();
    String generateErrorPage(const String& error);
    String generateFactoryResetPage();
    
    // CSS styles
    String getCSS();
};

#endif // CONFIG_PORTAL_H
