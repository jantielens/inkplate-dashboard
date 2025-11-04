#ifndef CONFIG_PORTAL_H
#define CONFIG_PORTAL_H

#include <Arduino.h>
#include <WebServer.h>
#include <Update.h>
#include "config_manager.h"
#include "wifi_manager.h"
#include "display_manager.h"

// Portal mode enum
enum PortalMode {
    BOOT_MODE,      // First boot - only WiFi credentials
    CONFIG_MODE     // Configuration mode - all settings including factory reset
};

class ConfigPortal {
public:
    ConfigPortal(ConfigManager* configManager, WiFiManager* wifiManager, DisplayManager* displayManager = nullptr);
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
    DisplayManager* _displayManager;
    WebServer* _server;
    bool _configReceived;
    int _port;
    PortalMode _mode;
    
    // HTTP handlers
    void handleRoot();
    void handleSubmit();
    void handleFactoryReset();
    void handleReboot();
    void handleOTA();
    void handleOTAUpload();
    void handleOTACheck();
    void handleOTAInstall();
    void handleOTAStatus();
    void handleOTAProgress();
    void handleNotFound();
    #ifndef DISPLAY_MODE_INKPLATE2
    void handleVcom();
    void handleVcomSubmit();
    #endif
    
    // HTML page generators
    String generateConfigPage();
    String generateSuccessPage();
    String generateErrorPage(const String& error);
    String generateFactoryResetPage();
    String generateRebootPage();
    String generateOTAPage();
    String generateOTAStatusPage();
    #ifndef DISPLAY_MODE_INKPLATE2
    String generateVcomPage(double currentVcom, const String& message = "", const String& diagnostics = "");
    #endif
    
    // CSS styles
    String getCSS();
    
    // Validation helpers
    bool validateIPv4Format(const String& ip);
};

#endif // CONFIG_PORTAL_H
