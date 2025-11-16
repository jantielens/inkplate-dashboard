// Mock configuration types for unit testing
#ifndef MOCK_CONFIG_H
#define MOCK_CONFIG_H

#include "Arduino.h"
#include <ctime>

// Mock WakeupReason enum
enum WakeupReason {
    WAKEUP_TIMER,
    WAKEUP_BUTTON,
    WAKEUP_FIRST_BOOT,
    WAKEUP_RESET_BUTTON,
    WAKEUP_UNKNOWN
};

// Carousel constraints
#define MAX_IMAGE_SLOTS 10
#define DEFAULT_INTERVAL_MINUTES 5

// Mock DashboardConfig struct
struct DashboardConfig {
    String wifiSSID;
    String wifiPassword;
    String friendlyName;
    String mqttBroker;
    String mqttUsername;
    String mqttPassword;
    bool isConfigured;
    bool debugMode;
    bool useCRC32Check;
    uint8_t updateHours[3];
    int timezoneOffset;
    uint8_t screenRotation;
    bool useStaticIP;
    String staticIP;
    String gateway;
    String subnet;
    String primaryDNS;
    String secondaryDNS;
    uint8_t imageCount;
    String imageUrls[MAX_IMAGE_SLOTS];
    int imageIntervals[MAX_IMAGE_SLOTS];
    bool imageStay[MAX_IMAGE_SLOTS];
    uint8_t frontlightDuration;
    uint8_t frontlightBrightness;
    
    DashboardConfig() : 
        wifiSSID(""),
        wifiPassword(""),
        friendlyName(""),
        mqttBroker(""),
        mqttUsername(""),
        mqttPassword(""),
        isConfigured(false),
        debugMode(false),
        useCRC32Check(false),
        timezoneOffset(0),
        screenRotation(0),
        useStaticIP(false),
        staticIP(""),
        gateway(""),
        subnet(""),
        primaryDNS(""),
        secondaryDNS(""),
        imageCount(0),
        frontlightDuration(0),
        frontlightBrightness(63) {
        updateHours[0] = 0xFF;
        updateHours[1] = 0xFF;
        updateHours[2] = 0xFF;
        
        for (int i = 0; i < MAX_IMAGE_SLOTS; i++) {
            imageUrls[i] = "";
            imageIntervals[i] = 0;
            imageStay[i] = false;
        }
    }
    
    bool isCarouselMode() const {
        return imageCount > 1;
    }
    
    int getAverageInterval() const {
        if (imageCount == 0) return DEFAULT_INTERVAL_MINUTES;
        int sum = 0;
        for (uint8_t i = 0; i < imageCount; i++) {
            sum += imageIntervals[i];
        }
        return sum / imageCount;
    }
};

// Mock other types needed for compilation
// Note: ConfigManager static helpers now use standalone functions from config_logic.h
class ConfigManager {
public:
    // These are just pass-through to standalone functions for backward compatibility
    static bool areAllHoursEnabled(const uint8_t updateHours[3]);
    static bool isHourEnabledInBitmask(uint8_t hour, const uint8_t updateHours[3]);
    static int applyTimezoneOffset(int hour, int offset);
};

class WiFiManager {};
class ImageManager {};
class PowerManager {};
class MQTTManager {};
class UIStatus {};
class UIError {};

#endif // MOCK_CONFIG_H
