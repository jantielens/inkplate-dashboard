#include "mqtt_manager.h"
#include "version.h"
#include "logger.h"
#include <WiFi.h>

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
    LogBox::begin("Initializing MQTT Manager");
    
    // Load MQTT configuration
    _broker = _configManager->getMQTTBroker();
    _username = _configManager->getMQTTUsername();
    _password = _configManager->getMQTTPassword();
    
    // Check if MQTT is configured
    if (_broker.length() == 0) {
        LogBox::line("MQTT not configured - skipping");
        LogBox::end();
        _isConfigured = false;
        return true;  // Not an error, just not configured
    }
    
    // Parse broker URL
    String host;
    if (!parseBrokerURL(_broker, host, _port)) {
        _lastError = "Invalid broker URL format";
        LogBox::line("ERROR: " + _lastError);
        LogBox::end();
        _isConfigured = false;
        return false;
    }
    
    LogBox::line("Broker: " + host + ":" + String(_port));
    LogBox::line("Username: " + (_username.length() > 0 ? _username : "(none)"));
    
    // Create MQTT client
    if (_mqttClient == nullptr) {
        _mqttClient = new PubSubClient(_wifiClient);
    }
    
    // Set buffer size for Home Assistant discovery messages
    _mqttClient->setBufferSize(MQTT_MAX_PACKET_SIZE);
    
    _mqttClient->setServer(host.c_str(), _port);
    _mqttClient->setKeepAlive(5);     // Optimized from 15s to 5s
    _mqttClient->setSocketTimeout(2); // Optimized from 10s to 2s
    
    _isConfigured = true;
    LogBox::end("MQTT Manager initialized successfully");
    
    return true;
}

bool MQTTManager::connect() {
    if (!_isConfigured) {
        LogBox::begin("MQTT Connection");
        LogBox::line("MQTT not configured - skipping connection");
        LogBox::end();
        return true;  // Not an error
    }
    
    if (_mqttClient == nullptr) {
        _lastError = "MQTT client not initialized";
        LogBox::begin("MQTT Connection");
        LogBox::line("ERROR: " + _lastError);
        LogBox::end();
        return false;
    }
    
    LogBox::begin("Connecting to MQTT broker");
    
    // Parse broker URL again to get host and port
    String host;
    int port;
    if (!parseBrokerURL(_broker, host, port)) {
        _lastError = "Failed to parse broker URL";
        LogBox::line("ERROR: " + _lastError);
        LogBox::end();
        return false;
    }
    
    LogBox::line("Broker Host: " + host);
    LogBox::linef("Broker Port: %d", port);
    
    // Generate unique client ID based on chip ID
    String clientId = "inkplate-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    LogBox::line("Client ID: " + clientId);
    
    // Set server again (ensure it's set correctly)
    _mqttClient->setServer(host.c_str(), port);
    _mqttClient->setKeepAlive(5);     // Optimized from 15s to 5s
    _mqttClient->setSocketTimeout(2); // Optimized from 10s to 2s
    
    // Attempt connection with retries
    const int maxRetries = 3;
    bool connected = false;
    
    for (int attempt = 1; attempt <= maxRetries && !connected; attempt++) {
        LogBox::linef("Connection attempt %d/%d...", attempt, maxRetries);
        
        if (_username.length() > 0) {
            LogBox::line("Connecting with username: " + _username);
            connected = _mqttClient->connect(clientId.c_str(), _username.c_str(), _password.c_str());
        } else {
            LogBox::line("Connecting without authentication");
            connected = _mqttClient->connect(clientId.c_str());
        }
        
        if (!connected) {
            int state = _mqttClient->state();
            LogBox::linef("Attempt %d failed. State: %d", attempt, state);
            
            // Decode MQTT error states
            switch (state) {
                case -4:
                    LogBox::line("  → MQTT_CONNECTION_TIMEOUT - Server didn't respond");
                    break;
                case -3:
                    LogBox::line("  → MQTT_CONNECTION_LOST - Network connection was broken");
                    break;
                case -2:
                    LogBox::line("  → MQTT_CONNECT_FAILED - Network connection failed");
                    LogBox::line("  → Check: Is the broker IP/hostname correct?");
                    LogBox::line("  → Check: Is the broker port correct? (usually 1883)");
                    LogBox::line("  → Check: Can you ping the broker from your network?");
                    break;
                case -1:
                    LogBox::line("  → MQTT_DISCONNECTED");
                    break;
                case 1:
                    LogBox::line("  → MQTT_CONNECT_BAD_PROTOCOL - Wrong protocol version");
                    break;
                case 2:
                    LogBox::line("  → MQTT_CONNECT_BAD_CLIENT_ID - Client ID rejected");
                    break;
                case 3:
                    LogBox::line("  → MQTT_CONNECT_UNAVAILABLE - Broker unavailable");
                    break;
                case 4:
                    LogBox::line("  → MQTT_CONNECT_BAD_CREDENTIALS - Username/password incorrect");
                    break;
                case 5:
                    LogBox::line("  → MQTT_CONNECT_UNAUTHORIZED - Not authorized");
                    break;
                default:
                    LogBox::linef("  → Unknown state: %d", state);
                    break;
            }
            
            if (attempt < maxRetries) {
                delay(1000);  // Wait 1 second before retry
            }
        }
    }
    
    if (connected) {
        LogBox::end("MQTT connected successfully!");
        return true;
    } else {
        _lastError = "Connection failed after " + String(maxRetries) + " attempts, state: " + String(_mqttClient->state());
        LogBox::line("ERROR: " + _lastError);
        LogBox::end();
        return false;
    }
}

void MQTTManager::disconnect() {
    if (_mqttClient != nullptr && _mqttClient->connected()) {
        _mqttClient->disconnect();
        LogBox::begin("MQTT");
        LogBox::line("Disconnected");
        LogBox::end();
    }
}

bool MQTTManager::publishDiscovery(const String& deviceId, const String& deviceName, const String& modelName,
                                     bool shouldPublish) {
    // Skip publishing if not needed
    if (!shouldPublish) {
        return true;  // Silent success
    }
    
    if (!_isConfigured || _mqttClient == nullptr || !_mqttClient->connected()) {
        return true;  // Skip if not configured or not connected
    }
    
    LogBox::begin("Publishing Home Assistant discovery");
    
    bool allSuccess = true;
    
    // Publish battery voltage sensor discovery
    {
        String discoveryTopic = getDiscoveryTopic(deviceId, "battery_voltage");
        String stateTopic = getStateTopic(deviceId, "battery_voltage");
        
        String payload = "{";
        payload += "\"name\":\"" + deviceName + " Battery\",";
        payload += "\"unique_id\":\"" + deviceId + "_battery\",";
        payload += "\"state_topic\":\"" + stateTopic + "\",";
        payload += "\"device_class\":\"voltage\",";
        payload += "\"unit_of_measurement\":\"V\",";
        payload += "\"value_template\":\"{{ value }}\",";
        payload += "\"device\":" + buildDeviceInfoJSON(deviceId, deviceName, modelName);
        payload += "}";
        
        LogBox::line("Battery Voltage Discovery:");
        LogBox::line("  Topic: " + discoveryTopic);
        LogBox::linef("  Payload: %d bytes", payload.length());
        
        if (!_mqttClient->publish(discoveryTopic.c_str(), payload.c_str(), true)) {
            LogBox::line("  ERROR: Failed to publish battery voltage discovery");
            allSuccess = false;
        } else {
            LogBox::line("  Success!");
        }
    }
    
    // Publish loop time sensor discovery
    {
        String discoveryTopic = getDiscoveryTopic(deviceId, "loop_time");
        String stateTopic = getStateTopic(deviceId, "loop_time");
        
        String payload = "{";
        payload += "\"name\":\"" + deviceName + " Loop Time\",";
        payload += "\"unique_id\":\"" + deviceId + "_loop_time\",";
        payload += "\"state_topic\":\"" + stateTopic + "\",";
        payload += "\"device_class\":\"duration\",";
        payload += "\"unit_of_measurement\":\"s\",";
        payload += "\"value_template\":\"{{ value }}\",";
        payload += "\"device\":" + buildDeviceInfoJSON(deviceId, deviceName, modelName);
        payload += "}";
        
        LogBox::line("Loop Time Discovery:");
        LogBox::line("  Topic: " + discoveryTopic);
        LogBox::linef("  Payload: %d bytes", payload.length());
        
        if (!_mqttClient->publish(discoveryTopic.c_str(), payload.c_str(), true)) {
            LogBox::line("  ERROR: Failed to publish loop time discovery");
            allSuccess = false;
        } else {
            LogBox::line("  Success!");
        }
    }
    
    // Publish WiFi signal sensor discovery
    {
        String discoveryTopic = getDiscoveryTopic(deviceId, "wifi_signal");
        String stateTopic = getStateTopic(deviceId, "wifi_signal");
        
        String payload = "{";
        payload += "\"name\":\"" + deviceName + " WiFi Signal\",";
        payload += "\"unique_id\":\"" + deviceId + "_wifi_signal\",";
        payload += "\"state_topic\":\"" + stateTopic + "\",";
        payload += "\"device_class\":\"signal_strength\",";
        payload += "\"unit_of_measurement\":\"dBm\",";
        payload += "\"value_template\":\"{{ value }}\",";
        payload += "\"device\":" + buildDeviceInfoJSON(deviceId, deviceName, modelName);
        payload += "}";
        
        LogBox::line("WiFi Signal Discovery:");
        LogBox::line("  Topic: " + discoveryTopic);
        LogBox::linef("  Payload: %d bytes", payload.length());
        
        if (!_mqttClient->publish(discoveryTopic.c_str(), payload.c_str(), true)) {
            LogBox::line("  ERROR: Failed to publish WiFi signal discovery");
            allSuccess = false;
        } else {
            LogBox::line("  Success!");
        }
    }
    
    // Publish last log message sensor discovery
    {
        String discoveryTopic = getDiscoveryTopic(deviceId, "last_log");
        String stateTopic = getStateTopic(deviceId, "last_log");
        
        String payload = "{";
        payload += "\"name\":\"" + deviceName + " Last Log\",";
        payload += "\"unique_id\":\"" + deviceId + "_last_log\",";
        payload += "\"state_topic\":\"" + stateTopic + "\",";
        payload += "\"icon\":\"mdi:message-text\",";
        payload += "\"value_template\":\"{{ value }}\",";
        payload += "\"device\":" + buildDeviceInfoJSON(deviceId, deviceName, modelName);
        payload += "}";
        
        LogBox::line("Last Log Discovery:");
        LogBox::line("  Topic: " + discoveryTopic);
        LogBox::linef("  Payload: %d bytes", payload.length());
        
        if (!_mqttClient->publish(discoveryTopic.c_str(), payload.c_str(), true)) {
            LogBox::line("  ERROR: Failed to publish last log discovery");
            allSuccess = false;
        } else {
            LogBox::line("  Success!");
        }
    }
    
    if (allSuccess) {
        LogBox::end("All discovery messages published successfully");
    } else {
        _lastError = "Some discovery messages failed to publish";
        LogBox::end("Some discovery messages failed");
    }
    
    return allSuccess;
}

bool MQTTManager::publishBatteryVoltage(const String& deviceId, float voltage) {
    if (!_isConfigured || _mqttClient == nullptr || !_mqttClient->connected()) {
        return true;  // Skip if not configured or not connected
    }
    
    LogBox::begin("Publishing battery voltage to MQTT");
    
    String stateTopic = getStateTopic(deviceId, "battery_voltage");
    String payload = String(voltage, 3);  // 3 decimal places
    
    LogBox::line("State Topic: " + stateTopic);
    LogBox::line("Voltage: " + payload + " V");
    
    bool success = _mqttClient->publish(stateTopic.c_str(), payload.c_str());
    
    if (success) {
        LogBox::end("Battery voltage published successfully");
    } else {
        _lastError = "Failed to publish battery voltage";
        LogBox::line("ERROR: " + _lastError);
        LogBox::end();
    }
    
    return success;
}

bool MQTTManager::publishLoopTime(const String& deviceId, float loopTimeSeconds) {
    if (!_isConfigured || _mqttClient == nullptr || !_mqttClient->connected()) {
        return true;  // Skip if not configured or not connected
    }
    
    LogBox::begin("Publishing loop time to MQTT");
    
    String stateTopic = getStateTopic(deviceId, "loop_time");
    String payload = String(loopTimeSeconds, 2);  // 2 decimal places
    
    LogBox::line("State Topic: " + stateTopic);
    LogBox::line("Loop Time: " + payload + " s");
    
    bool success = _mqttClient->publish(stateTopic.c_str(), payload.c_str());
    
    if (success) {
        LogBox::end("Loop time published successfully");
    } else {
        _lastError = "Failed to publish loop time";
        LogBox::line("ERROR: " + _lastError);
        LogBox::end();
    }
    
    return success;
}

bool MQTTManager::publishWiFiSignal(const String& deviceId, int rssi) {
    if (!_isConfigured || _mqttClient == nullptr || !_mqttClient->connected()) {
        return true;  // Skip if not configured or not connected
    }
    
    LogBox::begin("Publishing WiFi signal to MQTT");
    
    String stateTopic = getStateTopic(deviceId, "wifi_signal");
    String payload = String(rssi);  // Integer value
    
    LogBox::line("State Topic: " + stateTopic);
    LogBox::line("WiFi Signal: " + payload + " dBm");
    
    bool success = _mqttClient->publish(stateTopic.c_str(), payload.c_str());
    
    if (success) {
        LogBox::end("WiFi signal published successfully");
    } else {
        _lastError = "Failed to publish WiFi signal";
        LogBox::line("ERROR: " + _lastError);
        LogBox::end();
    }
    
    return success;
}

bool MQTTManager::publishLastLog(const String& deviceId, const String& message, const String& severity) {
    if (!_isConfigured || _mqttClient == nullptr || !_mqttClient->connected()) {
        return true;  // Skip if not configured or not connected
    }
    
    LogBox::begin("Publishing last log to MQTT");
    
    String stateTopic = getStateTopic(deviceId, "last_log");
    
    // Format: [SEVERITY] Message
    String severityUpper = severity;
    severityUpper.toUpperCase();
    String payload = "[" + severityUpper + "] " + message;
    
    LogBox::line("State Topic: " + stateTopic);
    LogBox::line("Message: " + payload);
    
    bool success = _mqttClient->publish(stateTopic.c_str(), payload.c_str());
    
    if (success) {
        LogBox::end("Last log published successfully");
    } else {
        _lastError = "Failed to publish last log";
        LogBox::line("ERROR: " + _lastError);
        LogBox::end();
    }
    
    return success;
}

bool MQTTManager::isConfigured() {
    return _isConfigured;
}

String MQTTManager::getLastError() {
    return _lastError;
}

bool MQTTManager::publishTelemetryBatch(const String& deviceId, const String& deviceName, const String& modelName,
                                        float batteryVoltage, int wifiRSSI, float loopTimeSeconds,
                                        bool shouldPublishDiscovery) {
    if (!_isConfigured) {
        return true;  // Skip if not configured
    }
    
    // Single connection for all publishing
    if (!connect()) {
        LogBox::begin("Telemetry Batch");
        LogBox::line("ERROR: Failed to connect to MQTT broker");
        LogBox::end();
        return false;
    }
    
    LogBox::begin("Publishing Telemetry Batch");
    LogBox::linef("Discovery: %s", shouldPublishDiscovery ? "YES" : "NO");
    
    bool allSuccess = true;
    
    // Publish discovery if needed
    if (shouldPublishDiscovery) {
        if (!publishDiscovery(deviceId, deviceName, modelName, true)) {
            allSuccess = false;
        }
    }
    
    // Publish battery voltage state
    if (!publishBatteryVoltage(deviceId, batteryVoltage)) {
        allSuccess = false;
    }
    
    // Publish WiFi signal state
    if (!publishWiFiSignal(deviceId, wifiRSSI)) {
        allSuccess = false;
    }
    
    // Publish loop time state
    if (!publishLoopTime(deviceId, loopTimeSeconds)) {
        allSuccess = false;
    }
    
    LogBox::end(allSuccess ? "Telemetry batch published successfully" : "Telemetry batch had some failures");
    
    // Single disconnect after all publishing
    disconnect();
    
    return allSuccess;
}

String MQTTManager::buildDeviceInfoJSON(const String& deviceId, const String& deviceName, const String& modelName) {
    String deviceInfo = "{";
    deviceInfo += "\"identifiers\":[\"" + deviceId + "\"],";
    deviceInfo += "\"name\":\"" + deviceName + "\",";
    deviceInfo += "\"manufacturer\":\"Soldered Electronics\",";
    deviceInfo += "\"model\":\"" + modelName + "\",";
    deviceInfo += "\"sw_version\":\"" + String(FIRMWARE_VERSION) + "\"";
    deviceInfo += "}";
    return deviceInfo;
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
