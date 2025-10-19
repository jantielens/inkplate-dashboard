#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include "config_manager.h"
#include "power_manager.h"  // For WakeupReason enum

// Increase MQTT buffer size for Home Assistant discovery messages
#define MQTT_MAX_PACKET_SIZE 512

class MQTTManager {
public:
    MQTTManager(ConfigManager* configManager);
    ~MQTTManager();
    
    // Initialize MQTT manager (loads config, sets up client)
    bool begin();
    
    // Connect to MQTT broker
    // Returns true if connected or connection not needed (no broker configured)
    bool connect();
    
    // Disconnect from MQTT broker
    void disconnect();
    
    // Publish Home Assistant auto-discovery configuration
    // deviceId: unique device identifier (e.g., chip ID)
    // deviceName: human-readable device name
    // modelName: board model name (e.g., "Inkplate 5 V2")
    bool publishDiscovery(const String& deviceId, const String& deviceName, const String& modelName);
    
    // Publish battery voltage to Home Assistant
    // deviceId: unique device identifier (must match discovery)
    // voltage: battery voltage in volts
    bool publishBatteryVoltage(const String& deviceId, float voltage);
    
    // Publish battery percentage to Home Assistant
    // deviceId: unique device identifier (must match discovery)
    // percentage: battery percentage (0-100)
    bool publishBatteryPercentage(const String& deviceId, int percentage);
    
    // Publish loop time to Home Assistant
    // deviceId: unique device identifier (must match discovery)
    // loopTimeSeconds: loop duration in seconds
    bool publishLoopTime(const String& deviceId, float loopTimeSeconds);
    
    // Publish WiFi signal strength to Home Assistant
    // deviceId: unique device identifier (must match discovery)
    // rssi: WiFi signal strength in dBm
    bool publishWiFiSignal(const String& deviceId, int rssi);
    
    // Publish last log message to Home Assistant
    // deviceId: unique device identifier (must match discovery)
    // message: log message text
    // severity: "info", "warning", or "error" (prefixed to message)
    bool publishLastLog(const String& deviceId, const String& message, const String& severity);
    
    // Publish all telemetry in a single MQTT session (optimized for battery-powered devices)
    // This method connects, publishes discovery (conditionally) + all state messages, then disconnects
    // deviceId: unique device identifier
    // deviceName: human-readable device name
    // modelName: board model name (e.g., "Inkplate 5 V2")
    // wakeReason: reason for waking (determines if discovery is published)
    // batteryVoltage: battery voltage in volts (0.0 to skip)
    // batteryPercentage: battery percentage (0-100, -1 to skip)
    // wifiRSSI: WiFi signal strength in dBm
    // loopTimeSeconds: loop duration in seconds
    // lastLogMessage: optional log message (empty to skip)
    // lastLogSeverity: "info", "warning", or "error"
    bool publishAllTelemetry(const String& deviceId, const String& deviceName, const String& modelName,
                             WakeupReason wakeReason, float batteryVoltage, int batteryPercentage,
                             int wifiRSSI, float loopTimeSeconds, const String& lastLogMessage = "", 
                             const String& lastLogSeverity = "info");
    
    // Check if MQTT is configured
    bool isConfigured();
    
    // Get last error message
    String getLastError();
    
private:
    ConfigManager* _configManager;
    WiFiClient _wifiClient;
    PubSubClient* _mqttClient;
    String _broker;
    String _username;
    String _password;
    int _port;
    String _lastError;
    bool _isConfigured;
    
    // Parse broker URL to extract host and port
    bool parseBrokerURL(const String& url, String& host, int& port);
    
    // Generate MQTT topics
    String getDiscoveryTopic(const String& deviceId, const String& sensorType);
    String getStateTopic(const String& deviceId, const String& sensorType);
    
    // Build device info JSON (reduces code duplication)
    String buildDeviceInfoJSON(const String& deviceId, const String& deviceName, const String& modelName, bool full);
    
    // Determine if discovery should be published based on wake reason
    bool shouldPublishDiscovery(WakeupReason wakeReason);
};

#endif // MQTT_MANAGER_H
