#ifndef AP_MODE_CONTROLLER_H
#define AP_MODE_CONTROLLER_H

#include <src/wifi_manager.h>
#include <src/config_portal.h>
#include <src/ui/ui_status.h>
#include <src/ui/ui_error.h>
#include <src/logger.h>

/**
 * @brief Handles Access Point mode for initial device configuration
 * 
 * This controller manages the AP mode which is activated on first boot
 * when the device has no WiFi configuration. It starts an access point
 * and configuration portal to allow users to enter WiFi credentials.
 */
class APModeController {
public:
    APModeController(WiFiManager* wifi, ConfigPortal* portal, UIStatus* uiStatus, UIError* uiError);
    
    /**
     * @brief Enter AP mode and start configuration portal
     * @return true if AP mode started successfully
     */
    bool begin();
    
    /**
     * @brief Handle AP mode client requests (call in loop)
     */
    void handleClient();
    
    /**
     * @brief Check if configuration was received
     * @return true if WiFi credentials were configured
     */
    bool isConfigReceived();
    
private:
    WiFiManager* wifiManager;
    ConfigPortal* configPortal;
    UIStatus* uiStatus;
    UIError* uiError;
};

#endif // AP_MODE_CONTROLLER_H
