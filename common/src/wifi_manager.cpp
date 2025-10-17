#include "wifi_manager.h"
#include "logger.h"

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
    LogBox::begin("Starting Access Point");
    LogBox::line("AP Name: " + _apName);
    
    // Disconnect from any WiFi network first
    WiFi.mode(WIFI_AP);
    
    // Start AP with no password (open network)
    bool success = WiFi.softAP(_apName.c_str());
    
    if (success) {
        _apActive = true;
        IPAddress ip = WiFi.softAPIP();
        LogBox::line("Access Point started successfully");
        LogBox::line("IP Address: " + ip.toString());
        LogBox::line("Connect to WiFi network: " + _apName);
        LogBox::line("Then navigate to: http://" + ip.toString());
        LogBox::end();
        return true;
    } else {
        LogBox::line("Failed to start Access Point");
        LogBox::end();
        _apActive = false;
        return false;
    }
}

void WiFiManager::stopAccessPoint() {
    if (_apActive) {
        LogBox::begin("Access Point");
        LogBox::line("Stopping Access Point...");
        LogBox::end();
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
    LogBox::begin("Connecting to WiFi");
    LogBox::line("SSID: " + ssid);
    
    // Stop AP mode if active
    if (_apActive) {
        stopAccessPoint();
    }
    
    // Switch to station mode
    WiFi.mode(WIFI_STA);
    
    // Set hostname for network identification
    String hostname = "inkplate-" + generateDeviceID();
    WiFi.setHostname(hostname.c_str());
    
    // Enable persistent credentials and auto-reconnect for faster connections
    WiFi.persistent(true);
    WiFi.setAutoReconnect(true);
    
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // Wait for connection with timeout
    unsigned long startTime = millis();
    int retries = 0;
    
    while (WiFi.status() != WL_CONNECTED && retries < WIFI_MAX_RETRIES) {
        if (millis() - startTime > WIFI_CONNECT_TIMEOUT_MS) {
            LogBox::line("Connection timeout, retrying...");
            WiFi.disconnect();
            delay(1000);
            WiFi.begin(ssid.c_str(), password.c_str());
            startTime = millis();
            retries++;
        }
        delay(100);
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.setSleep(false);  // Disable WiFi sleep to reduce latency during wake cycle
        LogBox::line("Connected to WiFi!");
        LogBox::line("IP Address: " + WiFi.localIP().toString());
        LogBox::linef("Signal Strength: %d dBm", WiFi.RSSI());
        LogBox::end();
        return true;
    } else {
        LogBox::linef("Failed to connect to WiFi after %d retries", WIFI_MAX_RETRIES);
        LogBox::end();
        return false;
    }
}

bool WiFiManager::connectToWiFi() {
    if (!_configManager) {
        LogBox::begin("WiFi Connection");
        LogBox::line("ConfigManager not set");
        LogBox::end();
        return false;
    }
    
    String ssid = _configManager->getWiFiSSID();
    String password = _configManager->getWiFiPassword();
    
    if (ssid.length() == 0) {
        LogBox::begin("WiFi Connection");
        LogBox::line("No WiFi credentials stored");
        LogBox::end();
        return false;
    }
    
    return connectToWiFi(ssid, password);
}

void WiFiManager::disconnect() {
    if (WiFi.status() == WL_CONNECTED) {
        LogBox::begin("WiFi");
        LogBox::line("Disconnecting from WiFi...");
        LogBox::end();
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
