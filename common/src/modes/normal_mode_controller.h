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
 * @brief Decision structure for image target determination
 */
struct ImageTargetDecision {
    uint8_t targetIndex;        // Which image to display (0-9)
    bool shouldAdvance;         // Should advance to next image after display
    const char* reason;         // Human-readable reason for this decision
};

/**
 * @brief Decision structure for CRC32 check behavior
 */
struct CRC32Decision {
    bool shouldCheck;           // Check CRC32 and potentially skip download (always fetch when enabled)
    const char* reason;         // Human-readable reason for this decision
};

/**
 * @brief Decision structure for sleep duration calculation
 */
struct SleepDecision {
    float sleepSeconds;         // How long to sleep (0 = indefinite/button-only)
    const char* reason;         // Human-readable reason for this duration
};

/**
 * @brief Structure to hold loop timing breakdown measurements
 */
struct LoopTimings {
    uint32_t wifi_ms = 0;
    uint32_t ntp_ms = 0;
    uint32_t crc_ms = 0;
    uint32_t image_ms = 0;
    
    // Retry counts for telemetry
    uint8_t wifi_retry_count = 0;   // WiFi connection retries (0-4)
    uint8_t crc_retry_count = 0;    // CRC32 check retries (0-2)
    uint8_t image_retry_count = 0;  // Image download retries (0-2, cross-sleep)
    
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
    
    // Decision functions - pure logic extraction for testability
    ImageTargetDecision determineImageTarget(const DashboardConfig& config, WakeupReason wakeReason, uint8_t currentIndex);
    CRC32Decision determineCRC32Action(const DashboardConfig& config, WakeupReason wakeReason, uint8_t currentIndex);
    SleepDecision determineSleepDuration(const DashboardConfig& config, time_t currentTime, uint8_t currentIndex, bool crc32Matched);
    
    // Helper methods
    bool loadConfiguration(DashboardConfig& config);
    int calculateSleepUntilNextEnabledHour(uint8_t currentHour, const uint8_t updateHours[3]);
    float calculateSleepMinutesToNextEnabledHour(time_t currentTime, int timezoneOffset, const uint8_t updateHours[3]);
    void publishMQTTTelemetry(const String& deviceId, const String& deviceName, WakeupReason wakeReason, float batteryVoltage, int batteryPercentage, int wifiRSSI, float loopTimeSeconds, uint32_t imageCRC32, const String& wifiBSSID, const LoopTimings& timings, const char* message = nullptr, const char* severity = nullptr);
    void handleImageSuccess(const DashboardConfig& config, uint32_t newCRC32, bool crc32WasChecked, bool crc32Matched, unsigned long loopStartTime, time_t currentTime, const String& deviceId, const String& deviceName, WakeupReason wakeReason, float batteryVoltage, int batteryPercentage, int wifiRSSI, const String& wifiBSSID, const LoopTimings& timings);
    void handleImageFailure(const DashboardConfig& config, unsigned long loopStartTime, time_t currentTime, const String& deviceId, const String& deviceName, WakeupReason wakeReason, float batteryVoltage, int batteryPercentage, int wifiRSSI, const String& wifiBSSID, const LoopTimings& timings);
    void handleWiFiFailure(const DashboardConfig& config, unsigned long loopStartTime);
};

#endif // NORMAL_MODE_CONTROLLER_H
