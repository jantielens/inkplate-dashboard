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
 * @brief Structure to hold loop timing breakdown measurements
 */
struct LoopTimings {
    uint32_t wifi_ms = 0;
    uint32_t ntp_ms = 0;
    uint32_t crc_ms = 0;
    uint32_t image_ms = 0;
    
    // Convert to seconds for MQTT publishing
    float wifiSeconds() const { return wifi_ms / 1000.0; }
    float ntpSeconds() const { return ntp_ms / 1000.0; }
    float crcSeconds() const { return crc_ms / 1000.0; }
    float imageSeconds() const { return image_ms / 1000.0; }
};

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
                        UIStatus* uiStatus, UIError* uiError, uint8_t* stateIndex);
    
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
    uint8_t* imageStateIndex;  // Pointer to RTC memory (carousel position or retry state)
    
    // Helper methods
    bool loadConfiguration(DashboardConfig& config);
    int calculateSleepUntilNextEnabledHour(uint8_t currentHour, const uint8_t updateHours[3]);
    float calculateSleepMinutesToNextEnabledHour(time_t currentTime, int timezoneOffset, const uint8_t updateHours[3]);
    bool checkAndHandleCRC32(const DashboardConfig& config, uint32_t& newCRC32, bool& crc32WasChecked, bool& crc32Matched, unsigned long loopStartTime, time_t currentTime, const String& deviceId, const String& deviceName, WakeupReason wakeReason, float batteryVoltage, int batteryPercentage, int wifiRSSI, const String& wifiBSSID, LoopTimings& timings);
    void publishMQTTTelemetry(const String& deviceId, const String& deviceName, WakeupReason wakeReason, float batteryVoltage, int batteryPercentage, int wifiRSSI, float loopTimeSeconds, uint32_t imageCRC32, const String& wifiBSSID, const LoopTimings& timings, const char* message = nullptr, const char* severity = nullptr);
    void handleImageSuccess(const DashboardConfig& config, uint32_t newCRC32, bool crc32WasChecked, bool crc32Matched, unsigned long loopStartTime, time_t currentTime, const String& deviceId, const String& deviceName, WakeupReason wakeReason, float batteryVoltage, int batteryPercentage, int wifiRSSI, const String& wifiBSSID, const LoopTimings& timings);
    void handleImageFailure(const DashboardConfig& config, unsigned long loopStartTime, time_t currentTime, const String& deviceId, const String& deviceName, WakeupReason wakeReason, float batteryVoltage, int batteryPercentage, int wifiRSSI, const String& wifiBSSID, const LoopTimings& timings);
    void handleWiFiFailure(const DashboardConfig& config, unsigned long loopStartTime);
    void enterSleep(const DashboardConfig& config, time_t currentTime, unsigned long loopStartTime);
};

#endif // NORMAL_MODE_CONTROLLER_H
