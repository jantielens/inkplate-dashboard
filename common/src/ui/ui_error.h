#ifndef UI_ERROR_H
#define UI_ERROR_H

#include <Arduino.h>
#include <src/display_manager.h>

/**
 * @brief Error screen rendering utilities
 * 
 * Provides consistent error message displays for various failure scenarios.
 */
class UIError {
public:
    UIError(DisplayManager* display);
    
    // Error screens
    void showWiFiError(const char* ssid, const char* status);
    void showImageError(const char* url, const char* error);
    void showAPStartError();
    void showPortalError();
    void showConfigLoadError();
    void showConfigModeFailure();
    
private:
    DisplayManager* displayManager;
};

#endif // UI_ERROR_H
