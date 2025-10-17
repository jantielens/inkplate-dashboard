#ifndef NORMAL_MODE_CONTROLLER_H
#define NORMAL_MODE_CONTROLLER_H

#include "Inkplate.h"
#include <src/config_manager.h>
#include <src/wifi_manager.h>
#include <src/image_manager.h>
#include <src/power_manager.h>
#include <src/mqtt_manager.h>
#include <src/ui/ui_status.h>
#include <src/ui/ui_error.h>
#include <src/logger.h>

/**
 * @brief Handles normal operation mode for image display
 * 
 * This controller manages the normal operation cycle:
 * - Connect to WiFi
 * - Publish MQTT telemetry
 * - Check CRC32 (if enabled)
 * - Download and display image
 * - Handle retry mechanism
 * - Enter deep sleep
 */
class NormalModeController {
public:
    NormalModeController(Inkplate* display, ConfigManager* config, WiFiManager* wifi,
                        ImageManager* image, PowerManager* power, MQTTManager* mqtt,
                        UIStatus* uiStatus, UIError* uiError, uint8_t* retryCount);
    
    /**
     * @brief Execute normal update cycle
     */
    void execute();
    
private:
    Inkplate* display;
    ConfigManager* configManager;
    WiFiManager* wifiManager;
    ImageManager* imageManager;
    PowerManager* powerManager;
    MQTTManager* mqttManager;
    UIStatus* uiStatus;
    UIError* uiError;
    uint8_t* imageRetryCount;  // Pointer to RTC memory
    
    // Helper methods
    bool loadConfiguration(DashboardConfig& config);
    bool connectWiFi(const DashboardConfig& config);
    bool publishMQTTTelemetry(const String& deviceId, const String& deviceName, int wifiRSSI, float batteryVoltage);
    bool checkAndHandleCRC32(const DashboardConfig& config, uint32_t& newCRC32, bool& crc32WasChecked, bool& crc32Matched, unsigned long loopStartTime);
    bool downloadAndDisplayImage(const DashboardConfig& config, bool showDebug, bool mqttSuccess);
    void handleImageSuccess(const DashboardConfig& config, uint32_t newCRC32, bool crc32WasChecked, bool crc32Matched, unsigned long loopStartTime, const String& deviceId, bool mqttConnected);
    void handleImageFailure(const DashboardConfig& config, unsigned long loopStartTime, const String& deviceId, bool mqttConnected);
    void handleWiFiFailure(const DashboardConfig& config, unsigned long loopStartTime);
    void enterSleep(uint16_t minutes);
};

#endif // NORMAL_MODE_CONTROLLER_H
