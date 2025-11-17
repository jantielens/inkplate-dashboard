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
    void showWiFiError(const char* ssid, const char* status);
    void showImageError(const char* url, const char* error);
    void showAPStartError();
    void showPortalError();
    void showConfigLoadError();
    void showConfigModeFailure();
};

#endif // UI_ERROR_H
