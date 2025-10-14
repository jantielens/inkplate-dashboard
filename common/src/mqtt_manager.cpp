#include "mqtt_manager.h"
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
    Serial.println("\n=================================");
    Serial.println("Initializing MQTT Manager...");
    Serial.println("=================================");
    
    // Load MQTT configuration
    _broker = _configManager->getMQTTBroker();
    _username = _configManager->getMQTTUsername();
    _password = _configManager->getMQTTPassword();
    
    // Check if MQTT is configured
    if (_broker.length() == 0) {
        Serial.println("MQTT not configured - skipping");
        _isConfigured = false;
        return true;  // Not an error, just not configured
    }
    
    // Parse broker URL
    String host;
    if (!parseBrokerURL(_broker, host, _port)) {
        _lastError = "Invalid broker URL format";
        Serial.println("ERROR: " + _lastError);
        _isConfigured = false;
        return false;
    }
    
    Serial.println("MQTT Configuration:");
    Serial.println("  Broker: " + host + ":" + String(_port));
    Serial.println("  Username: " + (_username.length() > 0 ? _username : "(none)"));
    
    // Create MQTT client
    if (_mqttClient == nullptr) {
        _mqttClient = new PubSubClient(_wifiClient);
    }
    
    // Set buffer size for Home Assistant discovery messages
    _mqttClient->setBufferSize(MQTT_MAX_PACKET_SIZE);
    
    _mqttClient->setServer(host.c_str(), _port);
    _mqttClient->setKeepAlive(15);
    _mqttClient->setSocketTimeout(10);
    
    _isConfigured = true;
    Serial.println("MQTT Manager initialized successfully");
    Serial.println("=================================\n");
    
    return true;
}

bool MQTTManager::connect() {
    if (!_isConfigured) {
        Serial.println("MQTT not configured - skipping connection");
        return true;  // Not an error
    }
    
    if (_mqttClient == nullptr) {
        _lastError = "MQTT client not initialized";
        Serial.println("ERROR: " + _lastError);
        return false;
    }
    
    Serial.println("\n=================================");
    Serial.println("Connecting to MQTT broker...");
    Serial.println("=================================");
    
    // Parse broker URL again to get host and port
    String host;
    int port;
    if (!parseBrokerURL(_broker, host, port)) {
        _lastError = "Failed to parse broker URL";
        Serial.println("ERROR: " + _lastError);
        Serial.println("=================================\n");
        return false;
    }
    
    Serial.printf("Broker Host: %s\n", host.c_str());
    Serial.printf("Broker Port: %d\n", port);
    
    // Generate unique client ID based on chip ID
    String clientId = "inkplate-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    Serial.println("Client ID: " + clientId);
    
    // Set server again (ensure it's set correctly)
    _mqttClient->setServer(host.c_str(), port);
    _mqttClient->setKeepAlive(15);
    _mqttClient->setSocketTimeout(10);
    
    // Attempt connection with retries
    const int maxRetries = 3;
    bool connected = false;
    
    for (int attempt = 1; attempt <= maxRetries && !connected; attempt++) {
        Serial.printf("Connection attempt %d/%d...\n", attempt, maxRetries);
        
        if (_username.length() > 0) {
            Serial.printf("Connecting with username: %s\n", _username.c_str());
            connected = _mqttClient->connect(clientId.c_str(), _username.c_str(), _password.c_str());
        } else {
            Serial.println("Connecting without authentication");
            connected = _mqttClient->connect(clientId.c_str());
        }
        
        if (!connected) {
            int state = _mqttClient->state();
            Serial.printf("Attempt %d failed. State: %d\n", attempt, state);
            
            // Decode MQTT error states
            switch (state) {
                case -4:
                    Serial.println("  → MQTT_CONNECTION_TIMEOUT - Server didn't respond");
                    break;
                case -3:
                    Serial.println("  → MQTT_CONNECTION_LOST - Network connection was broken");
                    break;
                case -2:
                    Serial.println("  → MQTT_CONNECT_FAILED - Network connection failed");
                    Serial.println("  → Check: Is the broker IP/hostname correct?");
                    Serial.println("  → Check: Is the broker port correct? (usually 1883)");
                    Serial.println("  → Check: Can you ping the broker from your network?");
                    break;
                case -1:
                    Serial.println("  → MQTT_DISCONNECTED");
                    break;
                case 1:
                    Serial.println("  → MQTT_CONNECT_BAD_PROTOCOL - Wrong protocol version");
                    break;
                case 2:
                    Serial.println("  → MQTT_CONNECT_BAD_CLIENT_ID - Client ID rejected");
                    break;
                case 3:
                    Serial.println("  → MQTT_CONNECT_UNAVAILABLE - Broker unavailable");
                    break;
                case 4:
                    Serial.println("  → MQTT_CONNECT_BAD_CREDENTIALS - Username/password incorrect");
                    break;
                case 5:
                    Serial.println("  → MQTT_CONNECT_UNAUTHORIZED - Not authorized");
                    break;
                default:
                    Serial.printf("  → Unknown state: %d\n", state);
                    break;
            }
            
            if (attempt < maxRetries) {
                delay(1000);  // Wait 1 second before retry
            }
        }
    }
    
    if (connected) {
        Serial.println("MQTT connected successfully!");
        Serial.println("=================================\n");
        return true;
    } else {
        _lastError = "Connection failed after " + String(maxRetries) + " attempts, state: " + String(_mqttClient->state());
        Serial.println("ERROR: " + _lastError);
        Serial.println("=================================\n");
        return false;
    }
}

void MQTTManager::disconnect() {
    if (_mqttClient != nullptr && _mqttClient->connected()) {
        _mqttClient->disconnect();
        Serial.println("MQTT disconnected");
    }
}

bool MQTTManager::publishDiscovery(const String& deviceId, const String& deviceName, const String& modelName) {
    if (!_isConfigured || _mqttClient == nullptr || !_mqttClient->connected()) {
        return true;  // Skip if not configured or not connected
    }
    
    Serial.println("\n=================================");
    Serial.println("Publishing Home Assistant discovery...");
    Serial.println("=================================");
    
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
        payload += "\"device\":{";
        payload += "\"identifiers\":[\"" + deviceId + "\"],";
        payload += "\"name\":\"" + deviceName + "\",";
        payload += "\"manufacturer\":\"Soldered Electronics\",";
        payload += "\"model\":\"" + modelName + "\"";
        payload += "}";
        payload += "}";
        
        Serial.println("Battery Voltage Discovery:");
        Serial.println("  Topic: " + discoveryTopic);
        Serial.println("  Payload: " + String(payload.length()) + " bytes");
        
        if (!_mqttClient->publish(discoveryTopic.c_str(), payload.c_str(), true)) {
            Serial.println("  ERROR: Failed to publish battery voltage discovery");
            allSuccess = false;
        } else {
            Serial.println("  Success!");
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
        payload += "\"device\":{";
        payload += "\"identifiers\":[\"" + deviceId + "\"]";
        payload += "}";
        payload += "}";
        
        Serial.println("Loop Time Discovery:");
        Serial.println("  Topic: " + discoveryTopic);
        Serial.println("  Payload: " + String(payload.length()) + " bytes");
        
        if (!_mqttClient->publish(discoveryTopic.c_str(), payload.c_str(), true)) {
            Serial.println("  ERROR: Failed to publish loop time discovery");
            allSuccess = false;
        } else {
            Serial.println("  Success!");
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
        payload += "\"device\":{";
        payload += "\"identifiers\":[\"" + deviceId + "\"]";
        payload += "}";
        payload += "}";
        
        Serial.println("WiFi Signal Discovery:");
        Serial.println("  Topic: " + discoveryTopic);
        Serial.println("  Payload: " + String(payload.length()) + " bytes");
        
        if (!_mqttClient->publish(discoveryTopic.c_str(), payload.c_str(), true)) {
            Serial.println("  ERROR: Failed to publish WiFi signal discovery");
            allSuccess = false;
        } else {
            Serial.println("  Success!");
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
        payload += "\"device\":{";
        payload += "\"identifiers\":[\"" + deviceId + "\"]";
        payload += "}";
        payload += "}";
        
        Serial.println("Last Log Discovery:");
        Serial.println("  Topic: " + discoveryTopic);
        Serial.println("  Payload: " + String(payload.length()) + " bytes");
        
        if (!_mqttClient->publish(discoveryTopic.c_str(), payload.c_str(), true)) {
            Serial.println("  ERROR: Failed to publish last log discovery");
            allSuccess = false;
        } else {
            Serial.println("  Success!");
        }
    }
    
    if (allSuccess) {
        Serial.println("All discovery messages published successfully");
    } else {
        _lastError = "Some discovery messages failed to publish";
    }
    Serial.println("=================================\n");
    
    return allSuccess;
}

bool MQTTManager::publishBatteryVoltage(const String& deviceId, float voltage) {
    if (!_isConfigured || _mqttClient == nullptr || !_mqttClient->connected()) {
        return true;  // Skip if not configured or not connected
    }
    
    Serial.println("\n=================================");
    Serial.println("Publishing battery voltage to MQTT...");
    Serial.println("=================================");
    
    String stateTopic = getStateTopic(deviceId, "battery_voltage");
    String payload = String(voltage, 3);  // 3 decimal places
    
    Serial.println("State Topic: " + stateTopic);
    Serial.println("Voltage: " + payload + " V");
    
    bool success = _mqttClient->publish(stateTopic.c_str(), payload.c_str());
    
    if (success) {
        Serial.println("Battery voltage published successfully");
        Serial.println("=================================\n");
    } else {
        _lastError = "Failed to publish battery voltage";
        Serial.println("ERROR: " + _lastError);
        Serial.println("=================================\n");
    }
    
    return success;
}

bool MQTTManager::publishLoopTime(const String& deviceId, float loopTimeSeconds) {
    if (!_isConfigured || _mqttClient == nullptr || !_mqttClient->connected()) {
        return true;  // Skip if not configured or not connected
    }
    
    Serial.println("\n=================================");
    Serial.println("Publishing loop time to MQTT...");
    Serial.println("=================================");
    
    String stateTopic = getStateTopic(deviceId, "loop_time");
    String payload = String(loopTimeSeconds, 2);  // 2 decimal places
    
    Serial.println("State Topic: " + stateTopic);
    Serial.println("Loop Time: " + payload + " s");
    
    bool success = _mqttClient->publish(stateTopic.c_str(), payload.c_str());
    
    if (success) {
        Serial.println("Loop time published successfully");
        Serial.println("=================================\n");
    } else {
        _lastError = "Failed to publish loop time";
        Serial.println("ERROR: " + _lastError);
        Serial.println("=================================\n");
    }
    
    return success;
}

bool MQTTManager::publishWiFiSignal(const String& deviceId, int rssi) {
    if (!_isConfigured || _mqttClient == nullptr || !_mqttClient->connected()) {
        return true;  // Skip if not configured or not connected
    }
    
    Serial.println("\n=================================");
    Serial.println("Publishing WiFi signal to MQTT...");
    Serial.println("=================================");
    
    String stateTopic = getStateTopic(deviceId, "wifi_signal");
    String payload = String(rssi);  // Integer value
    
    Serial.println("State Topic: " + stateTopic);
    Serial.println("WiFi Signal: " + payload + " dBm");
    
    bool success = _mqttClient->publish(stateTopic.c_str(), payload.c_str());
    
    if (success) {
        Serial.println("WiFi signal published successfully");
        Serial.println("=================================\n");
    } else {
        _lastError = "Failed to publish WiFi signal";
        Serial.println("ERROR: " + _lastError);
        Serial.println("=================================\n");
    }
    
    return success;
}

bool MQTTManager::publishLastLog(const String& deviceId, const String& message, const String& severity) {
    if (!_isConfigured || _mqttClient == nullptr || !_mqttClient->connected()) {
        return true;  // Skip if not configured or not connected
    }
    
    Serial.println("\n=================================");
    Serial.println("Publishing last log to MQTT...");
    Serial.println("=================================");
    
    String stateTopic = getStateTopic(deviceId, "last_log");
    
    // Format: [SEVERITY] Message
    String severityUpper = severity;
    severityUpper.toUpperCase();
    String payload = "[" + severityUpper + "] " + message;
    
    Serial.println("State Topic: " + stateTopic);
    Serial.println("Message: " + payload);
    
    bool success = _mqttClient->publish(stateTopic.c_str(), payload.c_str());
    
    if (success) {
        Serial.println("Last log published successfully");
        Serial.println("=================================\n");
    } else {
        _lastError = "Failed to publish last log";
        Serial.println("ERROR: " + _lastError);
        Serial.println("=================================\n");
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
