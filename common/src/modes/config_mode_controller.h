#ifndef CONFIG_MODE_CONTROLLER_H
#define CONFIG_MODE_CONTROLLER_H

#include <src/config_manager.h>
#include <src/wifi_manager.h>
#include <src/config_portal.h>
#include <src/mqtt_manager.h>
#include <src/power_manager.h>
#include <src/ui/ui_status.h>
#include <src/ui/ui_error.h>
#include <src/logger.h>

/**
 * @brief Handles Configuration mode for updating device settings
 * 
 * This controller manages the config mode which is activated by button press
 * or automatically when WiFi is configured but image URL is missing.
 * It connects to WiFi and serves the configuration portal.
 */
class ConfigModeController {
public:
    ConfigModeController(ConfigManager* config, WiFiManager* wifi, ConfigPortal* portal,
                        MQTTManager* mqtt, PowerManager* power,
                        UIStatus* uiStatus, UIError* uiError);
    
    // Set display pointer for battery reading
    void setDisplay(void* display);
    
    /**
     * @brief Enter config mode and start configuration portal
     * @return true if config mode started successfully
     */
    bool begin();
    
    /**
     * @brief Handle config mode client requests (call in loop)
     */
    void handleClient();
    
    /**
     * @brief Check if configuration was received
     * @return true if settings were updated
     */
    bool isConfigReceived();
    
    /**
     * @brief Check if config mode has timed out
     * @param startTime The time config mode was started
     * @return true if timeout occurred
     */
    bool isTimedOut(unsigned long startTime);
    
    /**
     * @brief Handle config mode timeout
     * @param refreshMinutes Default sleep duration in minutes
     */
    void handleTimeout(uint16_t refreshMinutes);
    
private:
    ConfigManager* configManager;
    WiFiManager* wifiManager;
    ConfigPortal* configPortal;
    MQTTManager* mqttManager;
    PowerManager* powerManager;
    UIStatus* uiStatus;
    UIError* uiError;
    void* display;
    
    bool hasPartialConfig;
    
    bool startConfigPortalWithWiFi(const String& localIP);
    bool startConfigPortalWithAP();
};

#endif // CONFIG_MODE_CONTROLLER_H
