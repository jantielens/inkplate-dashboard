#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include "config_manager.h"

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
};

#endif // MQTT_MANAGER_H
