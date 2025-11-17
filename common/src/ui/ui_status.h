#ifndef UI_STATUS_H
#define UI_STATUS_H

#include <Arduino.h>
#include <src/display_manager.h>
#include <src/overlay_manager.h>

/**
 * @brief Status and informational screen rendering utilities
 * 
 * Provides consistent status message displays for various operational states.
 */
class UIStatus {
public:
    UIStatus(DisplayManager* display);
    
    // Set overlay manager for battery icon rendering on logo screens
    void setOverlayManager(OverlayManager* overlayMgr);
    
    // AP Mode screens
    void showAPModeSetup(const char* apName, const char* apIP, const char* mdnsHostname = "", float batteryVoltage = 0.0);
    
    // Config Mode screens
    void showConfigModeSetup(const char* localIP, bool hasTimeout, int timeoutMinutes, const char* mdnsHostname = "", float batteryVoltage = 0.0);
    void showConfigModePartialSetup(const char* localIP, const char* mdnsHostname = "", float batteryVoltage = 0.0);
    void showConfigModeConnecting(const char* ssid, bool isPartialConfig, float batteryVoltage = 0.0);
    void showConfigModeWiFiFailed(const char* ssid, float batteryVoltage = 0.0);
    void showConfigModeAPFallback(const char* apName, const char* apIP, bool hasTimeout, int timeoutMinutes, const char* mdnsHostname = "", float batteryVoltage = 0.0);
    void showConfigModeTimeout();
    
    // Normal operation screens
    void showDebugStatus(const char* ssid, int refreshMinutes, float batteryVoltage = 0.0);
    void showDownloading(const char* url, bool mqttConnected, float batteryVoltage = 0.0);
    void showManualRefresh(float batteryVoltage = 0.0);
    
    // Success screens
    void showWiFiConfigured();
    void showSettingsUpdated();
    
private:
    DisplayManager* displayManager;
    OverlayManager* overlayManager;
    
    // Helper to draw battery icon at bottom left (for logo screens)
    void drawBatteryIconBottomLeft(float batteryVoltage);
};

#endif // UI_STATUS_H
