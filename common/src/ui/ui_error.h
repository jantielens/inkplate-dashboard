#ifndef UI_ERROR_H
#define UI_ERROR_H

#include <Arduino.h>
#include "ui_base.h"

/**
 * @brief Error screen rendering utilities
 * 
 * Provides consistent error message displays for various failure scenarios.
 */
class UIError : public UIBase {
public:
    UIError(DisplayManager* display);
    
    // Error screens
    void showWiFiError(const char* ssid, const char* status, float batteryVoltage = 0.0);
    void showImageError(const char* url, const char* error, float batteryVoltage = 0.0);
    void showAPStartError(float batteryVoltage = 0.0);
    void showPortalError(float batteryVoltage = 0.0);
    void showConfigLoadError(float batteryVoltage = 0.0);
    void showConfigModeFailure(float batteryVoltage = 0.0);
};

#endif // UI_ERROR_H
