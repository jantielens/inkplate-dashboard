#include "wifi_manager.h"

WiFiManager::WiFiManager(ConfigManager* configManager) 
    : _configManager(configManager), _apActive(false) {
    _apName = String(AP_SSID_PREFIX) + generateDeviceID();
}

WiFiManager::~WiFiManager() {
    if (_apActive) {
        stopAccessPoint();
    }
    disconnect();
}

String WiFiManager::generateDeviceID() {
    // Use MAC address last 6 digits as unique identifier
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char deviceID[8];
    snprintf(deviceID, sizeof(deviceID), "%02X%02X%02X", mac[3], mac[4], mac[5]);
    return String(deviceID);
}

bool WiFiManager::startAccessPoint() {
    Serial.println("Starting Access Point...");
    Serial.println("AP Name: " + _apName);
    
    // Disconnect from any WiFi network first
    WiFi.mode(WIFI_AP);
    
    // Start AP with no password (open network)
    bool success = WiFi.softAP(_apName.c_str());
    
    if (success) {
        _apActive = true;
        IPAddress ip = WiFi.softAPIP();
        Serial.println("Access Point started successfully");
        Serial.println("IP Address: " + ip.toString());
        Serial.println("Connect to WiFi network: " + _apName);
        Serial.println("Then navigate to: http://" + ip.toString());
        return true;
    } else {
        Serial.println("Failed to start Access Point");
        _apActive = false;
        return false;
    }
}

void WiFiManager::stopAccessPoint() {
    if (_apActive) {
        Serial.println("Stopping Access Point...");
        WiFi.softAPdisconnect(true);
        _apActive = false;
    }
}

String WiFiManager::getAPName() {
    return _apName;
}

String WiFiManager::getAPIPAddress() {
    if (_apActive) {
        return WiFi.softAPIP().toString();
    }
    return "0.0.0.0";
}

bool WiFiManager::isAPActive() {
    return _apActive;
}

bool WiFiManager::connectToWiFi(const String& ssid, const String& password) {
    Serial.println("Connecting to WiFi: " + ssid);
    
    // Stop AP mode if active
    if (_apActive) {
        stopAccessPoint();
    }
    
    // Switch to station mode
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // Wait for connection with timeout
    unsigned long startTime = millis();
    int retries = 0;
    
    while (WiFi.status() != WL_CONNECTED && retries < WIFI_MAX_RETRIES) {
        if (millis() - startTime > WIFI_CONNECT_TIMEOUT_MS) {
            Serial.println("Connection timeout, retrying...");
            WiFi.disconnect();
            delay(1000);
            WiFi.begin(ssid.c_str(), password.c_str());
            startTime = millis();
            retries++;
        }
        delay(100);
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to WiFi!");
        Serial.println("IP Address: " + WiFi.localIP().toString());
        Serial.println("Signal Strength: " + String(WiFi.RSSI()) + " dBm");
        return true;
    } else {
        Serial.println("Failed to connect to WiFi after " + String(WIFI_MAX_RETRIES) + " retries");
        return false;
    }
}

bool WiFiManager::connectToWiFi() {
    if (!_configManager) {
        Serial.println("ConfigManager not set");
        return false;
    }
    
    String ssid = _configManager->getWiFiSSID();
    String password = _configManager->getWiFiPassword();
    
    if (ssid.length() == 0) {
        Serial.println("No WiFi credentials stored");
        return false;
    }
    
    return connectToWiFi(ssid, password);
}

void WiFiManager::disconnect() {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Disconnecting from WiFi...");
        WiFi.disconnect();
    }
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String WiFiManager::getLocalIP() {
    if (isConnected()) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

int WiFiManager::getRSSI() {
    if (isConnected()) {
        return WiFi.RSSI();
    }
    return 0;
}

String WiFiManager::getStatusString() {
    switch (WiFi.status()) {
        case WL_CONNECTED:
            return "Connected";
        case WL_NO_SSID_AVAIL:
            return "SSID not available";
        case WL_CONNECT_FAILED:
            return "Connection failed";
        case WL_IDLE_STATUS:
            return "Idle";
        case WL_DISCONNECTED:
            return "Disconnected";
        default:
            return "Unknown";
    }
}
