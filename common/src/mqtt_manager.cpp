#include "mqtt_manager.h"
#include "version.h"
#include "logger.h"
#include <WiFi.h>

// Helper function to get concise MQTT state description
static const char* getMQTTStateDesc(int state) {
    switch (state) {
        case -4: return "Timeout - server didn't respond";
        case -3: return "Connection lost";
        case -2: return "Network failed - check broker IP/port";
        case -1: return "Disconnected";
        case 1: return "Bad protocol version";
        case 2: return "Client ID rejected";
        case 3: return "Broker unavailable";
        case 4: return "Bad credentials";
        case 5: return "Not authorized";
        default: return nullptr;
    }
}

MQTTManager::MQTTManager(ConfigManager* configManager)
    : _configManager(configManager), _mqttClient(nullptr), _port(1883), _isConfigured(false) {
}

MQTTManager::~MQTTManager() {
    disconnect();
    if (_mqttClient != nullptr) {
        delete _mqttClient;
        _mqttClient = nullptr;
    }
}

bool MQTTManager::begin() {
    Logger::begin("MQTT Init");
    
    // Load MQTT configuration
    _broker = _configManager->getMQTTBroker();
    _username = _configManager->getMQTTUsername();
    _password = _configManager->getMQTTPassword();
    
    // Check if MQTT is configured
    if (_broker.length() == 0) {
        Logger::end("Not configured - skipping");
        _isConfigured = false;
        return true;  // Not an error, just not configured
    }
    
    // Parse broker URL
    String host;
    if (!parseBrokerURL(_broker, host, _port)) {
        _lastError = "Invalid broker URL format";
        Logger::end("ERROR: " + _lastError);
        _isConfigured = false;
        return false;
    }
    
    Logger::linef("%s:%d (user: %s)", host.c_str(), _port, 
        _username.length() > 0 ? _username.c_str() : "none");
    
    // Create MQTT client
    if (_mqttClient == nullptr) {
        _mqttClient = new PubSubClient(_wifiClient);
    }
    
    // Set buffer size for Home Assistant discovery messages
    _mqttClient->setBufferSize(MQTT_MAX_PACKET_SIZE);
    
    _mqttClient->setServer(host.c_str(), _port);
    // Reduced timeouts for faster connection and publish cycles
    _mqttClient->setKeepAlive(5);     // Reduced from 15s to 5s
    _mqttClient->setSocketTimeout(2);  // Reduced from 10s to 2s
    
    _isConfigured = true;
    Logger::end();
    
    return true;
}

bool MQTTManager::connect() {
    if (!_isConfigured) {
        Logger::message("MQTT Connection", "MQTT not configured - skipping connection");
        return true;  // Not an error
    }
    
    if (_mqttClient == nullptr) {
        _lastError = "MQTT client not initialized";
        Logger::message("MQTT", "ERROR: " + _lastError);
        return false;
    }
    
    Logger::begin("MQTT Connect");
    
    // Parse broker URL again to get host and port
    String host;
    int port;
    if (!parseBrokerURL(_broker, host, port)) {
        _lastError = "Failed to parse broker URL";
        Logger::end("ERROR: " + _lastError);
        return false;
    }
    
    Logger::linef("%s:%d", host.c_str(), port);
    
    // Generate unique client ID based on chip ID
    String clientId = "inkplate-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    
    // Set server again (ensure it's set correctly)
    _mqttClient->setServer(host.c_str(), port);
    // Reduced timeouts for faster connection and publish cycles
    _mqttClient->setKeepAlive(5);     // Reduced from 15s to 5s
    _mqttClient->setSocketTimeout(2);  // Reduced from 10s to 2s
    
    // Attempt connection with retries
    const int maxRetries = 3;
    bool connected = false;
    
    for (int attempt = 1; attempt <= maxRetries && !connected; attempt++) {
        Logger::linef("Attempt %d/%d", attempt, maxRetries);
        
        if (_username.length() > 0) {
            connected = _mqttClient->connect(clientId.c_str(), _username.c_str(), _password.c_str());
        } else {
            connected = _mqttClient->connect(clientId.c_str());
        }
        
        if (!connected) {
            int state = _mqttClient->state();
            Logger::linef("Attempt %d failed (state: %d)", attempt, state);
            
            // Show concise error description
            const char* error = getMQTTStateDesc(state);
            if (error) {
                Logger::linef("  %s", error);
            }
            
            if (attempt < maxRetries) {
                delay(1000);  // Wait 1 second before retry
            }
        }
    }
    
    if (connected) {
        Logger::end("MQTT connected successfully!");
        return true;
    } else {
        _lastError = "Connection failed after " + String(maxRetries) + " attempts, state: " + String(_mqttClient->state());
        Logger::end("Failed: " + _lastError);
        return false;
    }
}

void MQTTManager::disconnect() {
    if (_mqttClient != nullptr && _mqttClient->connected()) {
        _mqttClient->disconnect();
    }
}

bool MQTTManager::publishDiscovery(const String& deviceId, const String& deviceName, const String& modelName) {
    if (!_isConfigured || _mqttClient == nullptr || !_mqttClient->connected()) {
        return true;  // Skip if not configured or not connected
    }
    
    Logger::begin("Publishing Home Assistant discovery");
    
    bool allSuccess = true;
    
    // Publish battery voltage sensor discovery
    {
        String discoveryTopic = getDiscoveryTopic(deviceId, "battery_voltage");
        
        Logger::line("Battery Voltage Discovery:");
        Logger::line("  Topic: " + discoveryTopic);
        
        if (!publishSensorDiscovery(discoveryTopic, deviceId, "battery_voltage", "Battery Voltage", 
                                   "voltage", "V", deviceName, modelName, true)) {
            Logger::line("  ERROR: Failed to publish battery voltage discovery");
            allSuccess = false;
        } else {
            Logger::line("  Success!");
        }
    }
    
    // Publish battery percentage sensor discovery
    {
        String discoveryTopic = getDiscoveryTopic(deviceId, "battery_percentage");
        
        Logger::line("Battery Percentage Discovery:");
        Logger::line("  Topic: " + discoveryTopic);
        
        if (!publishSensorDiscovery(discoveryTopic, deviceId, "battery_percentage", "Battery Percentage",
                                   "battery", "%", deviceName, modelName, false)) {
            Logger::line("  ERROR: Failed to publish battery percentage discovery");
            allSuccess = false;
        } else {
            Logger::line("  Success!");
        }
    }
    
    // Publish loop time sensor discovery
    {
        String discoveryTopic = getDiscoveryTopic(deviceId, "loop_time");
        
        Logger::line("Loop Time Discovery:");
        Logger::line("  Topic: " + discoveryTopic);
        
        if (!publishSensorDiscovery(discoveryTopic, deviceId, "loop_time", "Loop Time",
                                   "duration", "s", deviceName, modelName, false)) {
            Logger::line("  ERROR: Failed to publish loop time discovery");
            allSuccess = false;
        } else {
            Logger::line("  Success!");
        }
    }
    
    // Publish WiFi signal sensor discovery
    {
        String discoveryTopic = getDiscoveryTopic(deviceId, "wifi_signal");
        
        Logger::line("WiFi Signal Discovery:");
        Logger::line("  Topic: " + discoveryTopic);
        
        if (!publishSensorDiscovery(discoveryTopic, deviceId, "wifi_signal", "WiFi Signal",
                                   "signal_strength", "dBm", deviceName, modelName, false)) {
            Logger::line("  ERROR: Failed to publish WiFi signal discovery");
            allSuccess = false;
        } else {
            Logger::line("  Success!");
        }
    }
    
    // Publish last log message sensor discovery
    {
        String discoveryTopic = getDiscoveryTopic(deviceId, "last_log");
        
        Logger::line("Last Log Discovery:");
        Logger::line("  Topic: " + discoveryTopic);
        
        if (!publishSensorDiscovery(discoveryTopic, deviceId, "last_log", "Last Log", 
                                   "", "", deviceName, modelName, false)) {
            Logger::line("  ERROR: Failed to publish last log discovery");
            allSuccess = false;
        } else {
            Logger::line("  Success!");
        }
    }
    
    // Publish image CRC32 sensor discovery
    {
        String discoveryTopic = getDiscoveryTopic(deviceId, "image_crc32");
        
        Logger::line("Image CRC32 Discovery:");
        Logger::line("  Topic: " + discoveryTopic);
        
        if (!publishSensorDiscovery(discoveryTopic, deviceId, "image_crc32", "Image CRC32",
                                   "", "", deviceName, modelName, false)) {
            Logger::line("  ERROR: Failed to publish image CRC32 discovery");
            allSuccess = false;
        } else {
            Logger::line("  Success!");
        }
    }
    
    if (allSuccess) {
        Logger::end("All discovery messages published successfully");
    } else {
        _lastError = "Some discovery messages failed to publish";
        Logger::end("Some discovery messages failed");
    }
    
    return allSuccess;
}

bool MQTTManager::publishBatteryVoltage(const String& deviceId, float voltage) {
    if (!_isConfigured || _mqttClient == nullptr || !_mqttClient->connected()) {
        return true;  // Skip if not configured or not connected
    }
    
    Logger::begin("Publishing battery voltage to MQTT");
    
    String stateTopic = getStateTopic(deviceId, "battery_voltage");
    String payload = String(voltage, 3);  // 3 decimal places
    
    Logger::line("State Topic: " + stateTopic);
    Logger::line("Voltage: " + payload + " V");
    
    bool success = _mqttClient->publish(stateTopic.c_str(), payload.c_str());
    
    if (success) {
        Logger::end("Battery voltage published successfully");
    } else {
        _lastError = "Failed to publish battery voltage";
        Logger::line("ERROR: " + _lastError);
        Logger::end();
    }
    
    return success;
}

bool MQTTManager::publishBatteryPercentage(const String& deviceId, int percentage) {
    if (!_isConfigured || _mqttClient == nullptr || !_mqttClient->connected()) {
        return true;  // Skip if not configured or not connected
    }
    
    Logger::begin("Publishing battery percentage to MQTT");
    
    String stateTopic = getStateTopic(deviceId, "battery_percentage");
    String payload = String(percentage);
    
    Logger::line("State Topic: " + stateTopic);
    Logger::line("Percentage: " + payload + " %");
    
    bool success = _mqttClient->publish(stateTopic.c_str(), payload.c_str());
    
    if (success) {
        Logger::end("Battery percentage published successfully");
    } else {
        _lastError = "Failed to publish battery percentage";
        Logger::line("ERROR: " + _lastError);
        Logger::end();
    }
    
    return success;
}

bool MQTTManager::publishLoopTime(const String& deviceId, float loopTimeSeconds) {
    if (!_isConfigured || _mqttClient == nullptr || !_mqttClient->connected()) {
        return true;  // Skip if not configured or not connected
    }
    
    Logger::begin("Publishing loop time to MQTT");
    
    String stateTopic = getStateTopic(deviceId, "loop_time");
    String payload = String(loopTimeSeconds, 2);  // 2 decimal places
    
    Logger::line("State Topic: " + stateTopic);
    Logger::line("Loop Time: " + payload + " s");
    
    bool success = _mqttClient->publish(stateTopic.c_str(), payload.c_str());
    
    if (success) {
        Logger::end("Loop time published successfully");
    } else {
        _lastError = "Failed to publish loop time";
        Logger::line("ERROR: " + _lastError);
        Logger::end();
    }
    
    return success;
}

bool MQTTManager::publishWiFiSignal(const String& deviceId, int rssi) {
    if (!_isConfigured || _mqttClient == nullptr || !_mqttClient->connected()) {
        return true;  // Skip if not configured or not connected
    }
    
    Logger::begin("Publishing WiFi signal to MQTT");
    
    String stateTopic = getStateTopic(deviceId, "wifi_signal");
    String payload = String(rssi);  // Integer value
    
    Logger::line("State Topic: " + stateTopic);
    Logger::line("WiFi Signal: " + payload + " dBm");
    
    bool success = _mqttClient->publish(stateTopic.c_str(), payload.c_str());
    
    if (success) {
        Logger::end("WiFi signal published successfully");
    } else {
        _lastError = "Failed to publish WiFi signal";
        Logger::line("ERROR: " + _lastError);
        Logger::end();
    }
    
    return success;
}

bool MQTTManager::publishLastLog(const String& deviceId, const String& message, const String& severity) {
    if (!_isConfigured || _mqttClient == nullptr || !_mqttClient->connected()) {
        return true;  // Skip if not configured or not connected
    }
    
    Logger::begin("Publishing last log to MQTT");
    
    String stateTopic = getStateTopic(deviceId, "last_log");
    
    // Format: [SEVERITY] Message
    String severityUpper = severity;
    severityUpper.toUpperCase();
    String payload = "[" + severityUpper + "] " + message;
    
    Logger::line("State Topic: " + stateTopic);
    Logger::line("Message: " + payload);
    
    bool success = _mqttClient->publish(stateTopic.c_str(), payload.c_str());
    
    if (success) {
        Logger::end("Last log published successfully");
    } else {
        _lastError = "Failed to publish last log";
        Logger::line("ERROR: " + _lastError);
        Logger::end();
    }
    
    return success;
}

bool MQTTManager::publishImageCRC32(const String& deviceId, uint32_t crc32) {
    if (!_isConfigured || _mqttClient == nullptr || !_mqttClient->connected()) {
        return true;  // Skip if not configured or not connected
    }
    
    Logger::begin("Publishing image CRC32 to MQTT");
    
    String stateTopic = getStateTopic(deviceId, "image_crc32");
    String payload;
    
    // Format as hexadecimal string
    if (crc32 == 0) {
        payload = "0x00000000";
    } else {
        // Format as 0xHHHHHHHH
        char buffer[11];  // "0x" + 8 hex digits + null terminator
        snprintf(buffer, sizeof(buffer), "0x%08X", crc32);
        payload = String(buffer);
    }
    
    Logger::line("State Topic: " + stateTopic);
    Logger::line("CRC32: " + payload);
    
    bool success = _mqttClient->publish(stateTopic.c_str(), payload.c_str());
    
    if (success) {
        Logger::end("Image CRC32 published successfully");
    } else {
        _lastError = "Failed to publish image CRC32";
        Logger::line("ERROR: " + _lastError);
        Logger::end();
    }
    
    return success;
}

bool MQTTManager::isConfigured() {
    return _isConfigured;
}

String MQTTManager::getLastError() {
    return _lastError;
}

bool MQTTManager::parseBrokerURL(const String& url, String& host, int& port) {
    // Expected format: mqtt://broker.example.com:1883
    // or just: broker.example.com:1883
    // or just: broker.example.com (default port 1883)
    
    String workingUrl = url;
    
    // Remove mqtt:// prefix if present
    if (workingUrl.startsWith("mqtt://")) {
        workingUrl = workingUrl.substring(7);
    }
    
    // Find port separator
    int colonIndex = workingUrl.indexOf(':');
    
    if (colonIndex > 0) {
        // Port specified
        host = workingUrl.substring(0, colonIndex);
        String portStr = workingUrl.substring(colonIndex + 1);
        port = portStr.toInt();
        
        if (port <= 0 || port > 65535) {
            return false;  // Invalid port
        }
    } else {
        // No port specified, use default
        host = workingUrl;
        port = 1883;
    }
    
    // Validate host is not empty
    if (host.length() == 0) {
        return false;
    }
    
    return true;
}

String MQTTManager::getDiscoveryTopic(const String& deviceId, const String& sensorType) {
    // Home Assistant MQTT discovery topic format:
    // homeassistant/sensor/[device_id]/[sensor_type]/config
    return "homeassistant/sensor/" + deviceId + "/" + sensorType + "/config";
}

String MQTTManager::getStateTopic(const String& deviceId, const String& sensorType) {
    // State topic format:
    // homeassistant/sensor/[device_id]/[sensor_type]/state
    return "homeassistant/sensor/" + deviceId + "/" + sensorType + "/state";
}

bool MQTTManager::publishSensorDiscovery(const String& discoveryTopic, const String& deviceId, const String& sensorType,
                                         const String& name, const String& deviceClass, const String& unit,
                                         const String& deviceName, const String& modelName, bool includeFullDevice) {
    String stateTopic = getStateTopic(deviceId, sensorType);
    
    String payload = "{";
    payload += "\"name\":\"" + name + "\",";
    payload += "\"unique_id\":\"" + deviceId + "_" + sensorType + "\",";
    payload += "\"state_topic\":\"" + stateTopic + "\",";
    
    if (deviceClass.length() > 0) {
        payload += "\"device_class\":\"" + deviceClass + "\",";
    }
    
    if (unit.length() > 0) {
        payload += "\"unit_of_measurement\":\"" + unit + "\",";
    }
    
    // Check if this is a special sensor (last_log doesn't have device_class/unit)
    if (sensorType == "last_log") {
        payload += "\"icon\":\"mdi:message-text\",";
    }
    
    // Force update for all sensors EXCEPT event-like sensors (last_log)
    // This ensures Home Assistant updates "last seen" timestamp even when values don't change
    // Benefits:
    // - Reliable device health monitoring (stable battery/WiFi values still show device is alive)
    // - Retry fields (often zero) are visible as successful states
    // - Reduces confusion when stable sensors appear inactive but are actively reporting
    // Event sensors like last_log are excluded because they represent discrete events,
    // not continuous telemetry, and repeating the same event message is not useful
    if (sensorType != "last_log") {
        payload += "\"force_update\":true,";
    }
    
    payload += "\"value_template\":\"{{ value }}\",";
    payload += buildDeviceInfoJSON(deviceId, deviceName, modelName, includeFullDevice);
    payload += "}";
    
    return _mqttClient->publish(discoveryTopic.c_str(), payload.c_str(), true);
}

String MQTTManager::buildDeviceInfoJSON(const String& deviceId, const String& deviceName, const String& modelName, bool full) {
    // Build device info JSON
    // full=true: includes all device metadata (for first sensor)
    // full=false: includes only identifiers (for subsequent sensors)
    String json = "\"device\":{";
    json += "\"identifiers\":[\"" + deviceId + "\"]";
    
    if (full) {
        json += ",\"name\":\"" + deviceName + "\"";
        json += ",\"manufacturer\":\"Inkplate Dashboard\"";
        json += ",\"model\":\"" + modelName + "\"";
        json += ",\"sw_version\":\"" + String(FIRMWARE_VERSION) + "\"";
    }
    
    json += "}";
    return json;
}

bool MQTTManager::shouldPublishDiscovery(WakeupReason wakeReason) {
    // Only publish discovery on:
    // - WAKEUP_FIRST_BOOT: Initial setup, ensures HA entities are created
    // - WAKEUP_RESET_BUTTON: Hardware reset, may indicate config/firmware change
    //
    // DO NOT publish on:
    // - WAKEUP_TIMER: Normal refresh cycle (most common)
    // - WAKEUP_BUTTON: Manual refresh, no config change
    //
    // This significantly reduces MQTT traffic and retained message count
    return (wakeReason == WAKEUP_FIRST_BOOT || wakeReason == WAKEUP_RESET_BUTTON);
}

bool MQTTManager::publishAllTelemetry(const String& deviceId, const String& deviceName, const String& modelName,
                                      WakeupReason wakeReason, float batteryVoltage, int batteryPercentage,
                                      int wifiRSSI, float loopTimeSeconds, uint32_t imageCRC32,
                                      const String& lastLogMessage, const String& lastLogSeverity,
                                      const String& wifiBSSID,
                                      float wifiTimeSeconds, float ntpTimeSeconds, 
                                      float crcTimeSeconds, float imageTimeSeconds,
                                      uint8_t wifiRetryCount, uint8_t crcRetryCount, uint8_t imageRetryCount) {
    if (!_isConfigured) {
        Logger::message("MQTT", "MQTT not configured - skipping");
        return true;  // Not an error
    }
    
    Logger::begin("Publishing All Telemetry to MQTT");
    Logger::line("Connecting to MQTT broker...");
    
    // Connect to MQTT
    if (!connect()) {
        Logger::line("ERROR: Failed to connect to MQTT broker");
        Logger::line("Error: " + _lastError);
        Logger::end();
        return false;
    }
    
    Logger::line("Connected successfully");
    
    int publishCount = 0;
    
    // Publish discovery messages (conditionally based on wake reason)
    if (shouldPublishDiscovery(wakeReason)) {
        Logger::line("Publishing discovery messages...");
        
        // Battery voltage sensor discovery
        publishSensorDiscovery(getDiscoveryTopic(deviceId, "battery_voltage"), deviceId, "battery_voltage",
                              "Battery Voltage", "voltage", "V", deviceName, modelName, true);
        publishCount++;
        
        // Battery percentage sensor discovery
        publishSensorDiscovery(getDiscoveryTopic(deviceId, "battery_percentage"), deviceId, "battery_percentage",
                              "Battery Percentage", "battery", "%", deviceName, modelName, false);
        publishCount++;
        
        // Loop time sensor discovery
        publishSensorDiscovery(getDiscoveryTopic(deviceId, "loop_time"), deviceId, "loop_time",
                              "Loop Time", "duration", "s", deviceName, modelName, false);
        publishCount++;
        
        // WiFi signal sensor discovery
        publishSensorDiscovery(getDiscoveryTopic(deviceId, "wifi_signal"), deviceId, "wifi_signal",
                              "WiFi Signal", "signal_strength", "dBm", deviceName, modelName, false);
        publishCount++;
        
        // Last log sensor discovery
        publishSensorDiscovery(getDiscoveryTopic(deviceId, "last_log"), deviceId, "last_log",
                              "Last Log", "", "", deviceName, modelName, false);
        publishCount++;
        
        // Image CRC32 sensor discovery
        publishSensorDiscovery(getDiscoveryTopic(deviceId, "image_crc32"), deviceId, "image_crc32",
                              "Image CRC32", "", "", deviceName, modelName, false);
        publishCount++;
        
        // WiFi BSSID sensor discovery
        publishSensorDiscovery(getDiscoveryTopic(deviceId, "wifi_bssid"), deviceId, "wifi_bssid",
                              "WiFi BSSID", "", "", deviceName, modelName, false);
        publishCount++;
        
        // Loop time breakdown sensor discoveries
        publishSensorDiscovery(getDiscoveryTopic(deviceId, "loop_time_wifi"), deviceId, "loop_time_wifi",
                              "Loop Time - WiFi", "duration", "s", deviceName, modelName, false);
        publishCount++;
        
        publishSensorDiscovery(getDiscoveryTopic(deviceId, "loop_time_ntp"), deviceId, "loop_time_ntp",
                              "Loop Time - NTP", "duration", "s", deviceName, modelName, false);
        publishCount++;
        
        publishSensorDiscovery(getDiscoveryTopic(deviceId, "loop_time_crc"), deviceId, "loop_time_crc",
                              "Loop Time - CRC", "duration", "s", deviceName, modelName, false);
        publishCount++;
        
        publishSensorDiscovery(getDiscoveryTopic(deviceId, "loop_time_image"), deviceId, "loop_time_image",
                              "Loop Time - Image", "duration", "s", deviceName, modelName, false);
        publishCount++;
        
        // Retry count sensor discoveries
        publishSensorDiscovery(getDiscoveryTopic(deviceId, "loop_time_wifi_retries"), deviceId, "loop_time_wifi_retries",
                              "Loop Time - WiFi Retries", "", "", deviceName, modelName, false);
        publishCount++;
        
        publishSensorDiscovery(getDiscoveryTopic(deviceId, "loop_time_crc_retries"), deviceId, "loop_time_crc_retries",
                              "Loop Time - CRC Retries", "", "", deviceName, modelName, false);
        publishCount++;
        
        publishSensorDiscovery(getDiscoveryTopic(deviceId, "loop_time_image_retries"), deviceId, "loop_time_image_retries",
                              "Loop Time - Image Retries", "", "", deviceName, modelName, false);
        publishCount++;
        
        Logger::linef("Published %d discovery messages", publishCount);
        publishCount = 0;  // Reset for state messages
    } else {
        Logger::line("Skipping discovery (normal wake cycle)");
    }
    
    // Publish battery voltage state
    if (batteryVoltage > 0.0) {
        String stateTopic = getStateTopic(deviceId, "battery_voltage");
        String payload = String(batteryVoltage, 3);
        _mqttClient->publish(stateTopic.c_str(), payload.c_str(), true);
        Logger::line("Battery: " + payload + " V");
        publishCount++;
    }
    
    // Publish battery percentage state
    if (batteryPercentage >= 0) {
        String stateTopic = getStateTopic(deviceId, "battery_percentage");
        String payload = String(batteryPercentage);
        _mqttClient->publish(stateTopic.c_str(), payload.c_str(), true);
        Logger::line("Battery Percentage: " + payload + " %");
        publishCount++;
    }
    
    // Publish WiFi signal state
    {
        String stateTopic = getStateTopic(deviceId, "wifi_signal");
        String payload = String(wifiRSSI);
        _mqttClient->publish(stateTopic.c_str(), payload.c_str(), true);
        Logger::line("WiFi Signal: " + payload + " dBm");
        publishCount++;
    }
    
    // Publish loop time state
    {
        String stateTopic = getStateTopic(deviceId, "loop_time");
        String payload = String(loopTimeSeconds, 2);
        _mqttClient->publish(stateTopic.c_str(), payload.c_str(), true);
        Logger::line("Loop Time: " + payload + " s");
        publishCount++;
    }
    
    // Publish last log state (if provided)
    if (lastLogMessage.length() > 0) {
        String stateTopic = getStateTopic(deviceId, "last_log");
        String severityUpper = lastLogSeverity;
        severityUpper.toUpperCase();
        String payload = "[" + severityUpper + "] " + lastLogMessage;
        _mqttClient->publish(stateTopic.c_str(), payload.c_str(), true);
        Logger::line("Last Log: " + payload);
        publishCount++;
    }
    
    // Publish image CRC32 state
    {
        String stateTopic = getStateTopic(deviceId, "image_crc32");
        String payload;
        
        // Format as hexadecimal string
        if (imageCRC32 == 0) {
            payload = "0x00000000";
        } else {
            // Format as 0xHHHHHHHH
            char buffer[11];  // "0x" + 8 hex digits + null terminator
            snprintf(buffer, sizeof(buffer), "0x%08X", imageCRC32);
            payload = String(buffer);
        }
        
        _mqttClient->publish(stateTopic.c_str(), payload.c_str(), true);
        Logger::line("Image CRC32: " + payload);
        publishCount++;
    }
    
    // Publish WiFi BSSID state (if provided)
    if (wifiBSSID.length() > 0) {
        String stateTopic = getStateTopic(deviceId, "wifi_bssid");
        _mqttClient->publish(stateTopic.c_str(), wifiBSSID.c_str(), true);
        Logger::line("WiFi BSSID: " + wifiBSSID);
        publishCount++;
    }
    
    // Publish loop time breakdown states (always publish for correlation, 0 = skipped)
    if (wifiTimeSeconds >= 0) {
        String stateTopic = getStateTopic(deviceId, "loop_time_wifi");
        String payload = String(wifiTimeSeconds, 2);
        _mqttClient->publish(stateTopic.c_str(), payload.c_str(), true);
        Logger::line("Loop Time - WiFi: " + payload + " s");
        publishCount++;
    }
    
    if (ntpTimeSeconds >= 0) {
        String stateTopic = getStateTopic(deviceId, "loop_time_ntp");
        String payload = String(ntpTimeSeconds, 2);
        _mqttClient->publish(stateTopic.c_str(), payload.c_str(), true);
        Logger::line("Loop Time - NTP: " + payload + " s");
        publishCount++;
    }
    
    if (crcTimeSeconds >= 0) {
        String stateTopic = getStateTopic(deviceId, "loop_time_crc");
        String payload = String(crcTimeSeconds, 2);
        _mqttClient->publish(stateTopic.c_str(), payload.c_str(), true);
        Logger::line("Loop Time - CRC: " + payload + " s");
        publishCount++;
    }
    
    if (imageTimeSeconds >= 0) {
        String stateTopic = getStateTopic(deviceId, "loop_time_image");
        String payload = String(imageTimeSeconds, 2);
        _mqttClient->publish(stateTopic.c_str(), payload.c_str(), true);
        Logger::line("Loop Time - Image: " + payload + " s");
        publishCount++;
    }
    
    // Publish retry counts (255 means skip)
    if (wifiRetryCount != 255) {
        String stateTopic = getStateTopic(deviceId, "loop_time_wifi_retries");
        String payload = String(wifiRetryCount);
        _mqttClient->publish(stateTopic.c_str(), payload.c_str(), true);
        Logger::line("Loop Time - WiFi Retries: " + payload);
        publishCount++;
    }
    
    if (crcRetryCount != 255) {
        String stateTopic = getStateTopic(deviceId, "loop_time_crc_retries");
        String payload = String(crcRetryCount);
        _mqttClient->publish(stateTopic.c_str(), payload.c_str(), true);
        Logger::line("Loop Time - CRC Retries: " + payload);
        publishCount++;
    }
    
    if (imageRetryCount != 255) {
        String stateTopic = getStateTopic(deviceId, "loop_time_image_retries");
        String payload = String(imageRetryCount);
        _mqttClient->publish(stateTopic.c_str(), payload.c_str(), true);
        Logger::line("Loop Time - Image Retries: " + payload);
        publishCount++;
    }
    
    Logger::linef("Published %d state messages", publishCount);
    
    // Give MQTT client time to transmit all queued messages
    // PubSubClient needs loop() calls to actually send queued data
    // 20-30ms is typically sufficient for transmission
    for (int i = 0; i < 3; i++) {
        _mqttClient->loop();
        delay(10);
    }
    
    // Disconnect
    disconnect();
    
    Logger::end("All telemetry published");
    
    return true;
}
