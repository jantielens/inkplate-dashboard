#ifndef UI_STATUS_H
#define UI_STATUS_H

#include <Arduino.h>
#include <src/display_manager.h>

/**
 * @brief Status and informational screen rendering utilities
 * 
 * Provides consistent status message displays for various operational states.
 */
class UIStatus {
public:
    UIStatus(DisplayManager* display);
    
    // AP Mode screens
    void showAPModeSetup(const char* apName, const char* apIP);
    
    // Config Mode screens
    void showConfigModeSetup(const char* localIP, bool hasTimeout, int timeoutMinutes);
    void showConfigModePartialSetup(const char* localIP);
    void showConfigModeConnecting(const char* ssid, bool isPartialConfig);
    void showConfigModeWiFiFailed(const char* ssid);
    void showConfigModeAPFallback(const char* apName, const char* apIP, bool hasTimeout, int timeoutMinutes);
    void showConfigModeTimeout();
    
    // Normal operation screens
    void showDebugStatus(const char* ssid, int refreshMinutes);
    void showDownloading(const char* url, bool mqttConnected);
    void showManualRefresh();
    
    // Success screens
    void showWiFiConfigured();
    void showSettingsUpdated();
    
private:
    DisplayManager* displayManager;
};

#endif // UI_STATUS_H
