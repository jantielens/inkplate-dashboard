#include "wifi_manager.h"
#include "logger.h"

WiFiManager::WiFiManager(ConfigManager* configManager) 
    : _configManager(configManager), _powerManager(nullptr), _apActive(false) {
    _apName = String(AP_SSID_PREFIX) + generateDeviceID();
}

void WiFiManager::setPowerManager(PowerManager* powerManager) {
    _powerManager = powerManager;
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

String WiFiManager::getDeviceIdentifier() {
    // Try to get friendly name from config
    if (_configManager) {
        String friendlyName = _configManager->getFriendlyName();
        
        // Validate the friendly name using sanitization
        String sanitized;
        if (friendlyName.length() > 0 && 
            ConfigManager::sanitizeFriendlyName(friendlyName, sanitized) && 
            sanitized.length() > 0) {
            return sanitized;
        }
    }
    
    // Fallback to MAC-based identifier (use full 32-bit MAC for backward compatibility)
    // This matches the old format: "inkplate-" + 8 hex chars from ESP.getEfuseMac()
    return "inkplate-" + String((uint32_t)ESP.getEfuseMac(), HEX);
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
        LogBox::message("Access Point", "Stopping Access Point...");
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

bool WiFiManager::connectToWiFi(const String& ssid, const String& password, uint8_t* outRetryCount) {
    LogBox::begin("Connecting to WiFi");
    LogBox::line("SSID: " + ssid);
    
    // Initialize retry count
    uint8_t retryCount = 0;
    
    // Stop AP mode if active
    if (_apActive) {
        stopAccessPoint();
    }
    
    // Switch to station mode
    WiFi.mode(WIFI_STA);
    
    // Set hostname for network identification (uses friendly name if set)
    String hostname = getDeviceIdentifier();
    WiFi.setHostname(hostname.c_str());
    
    // Enable persistent credentials and auto-reconnect for faster connections
    WiFi.persistent(true);
    WiFi.setAutoReconnect(true);
    
    // Check if static IP is configured
    if (_configManager && _configManager->getUseStaticIP()) {
        String staticIP = _configManager->getStaticIP();
        String gateway = _configManager->getGateway();
        String subnet = _configManager->getSubnet();
        String dns1 = _configManager->getPrimaryDNS();
        String dns2 = _configManager->getSecondaryDNS();
        
        if (!configureStaticIP(staticIP, gateway, subnet, dns1, dns2)) {
            LogBox::line("Failed to configure static IP, connection aborted");
            LogBox::end();
            if (outRetryCount) *outRetryCount = retryCount;
            return false;
        }
    } else {
        LogBox::line("Network mode: DHCP");
    }
    
    // Determine connection strategy based on wake reason
    bool useChannelLock = false;
    bool shouldSaveChannel = false;
    WakeupReason wakeReason = WAKEUP_FIRST_BOOT;
    
    if (_powerManager) {
        wakeReason = _powerManager->getWakeupReason();
        
        // Use channel lock only for timer wakeups (optimization for regular updates)
        if (wakeReason == WAKEUP_TIMER && _configManager && _configManager->hasWiFiChannelLock()) {
            useChannelLock = true;
            LogBox::line("Wake reason: Timer - using channel lock for fast connect");
        } else {
            // For boot, reset, or button wakeups, do full scan and save new channel/BSSID
            shouldSaveChannel = true;
            const char* wakeReasonStr = 
                wakeReason == WAKEUP_FIRST_BOOT ? "First boot" :
                wakeReason == WAKEUP_RESET_BUTTON ? "Reset button" :
                wakeReason == WAKEUP_BUTTON ? "Button press" : "Timer (no lock)";
            LogBox::linef("Wake reason: %s - performing full scan", wakeReasonStr);
        }
    } else {
        LogBox::line("PowerManager not set - using full scan");
        shouldSaveChannel = true;
    }
    
    // Try connection with channel lock (fast path)
    if (useChannelLock) {
        uint8_t channel = _configManager->getWiFiChannel();
        uint8_t bssid[6];
        _configManager->getWiFiBSSID(bssid);
        
        LogBox::linef("Attempting channel-locked connection (channel %d)", channel);
        WiFi.begin(ssid.c_str(), password.c_str(), channel, bssid);
        
        // Wait with shorter timeout for channel-locked connection
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < 2000) {
            delay(10);  // Reduced polling interval for faster response
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            WiFi.setSleep(false);
            LogBox::line("Fast connect successful!");
            LogBox::line("IP Address: " + WiFi.localIP().toString());
            LogBox::linef("Signal Strength: %d dBm", WiFi.RSSI());
            LogBox::end();
            if (outRetryCount) *outRetryCount = 0;  // Channel lock success = 0 retries
            return true;
        }
        
        // Channel lock failed - fall back to full scan
        // This counts as the first retry (fallback to full scan)
        LogBox::line("Channel lock failed (network moved/changed) - falling back to full scan");
        WiFi.disconnect();
        delay(100);
        shouldSaveChannel = true;  // Save new channel after successful full scan
    }
    
    // Full scan connection (slower but more reliable)
    // Optimized timeouts: 3s per attempt (down from 5s), 300ms retry delay (down from 1s)
    // Max retries: 4 (up from 3) to compensate for shorter timeout
    // Total max time: 3s + 0.3s + 3s + 0.3s + 3s + 0.3s + 3s + 0.3s + 3s = 16.2s (vs 23s previously)
    LogBox::line("Performing full network scan...");
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // Wait for connection with optimized timeout
    unsigned long startTime = millis();
    int fullScanRetries = 0;  // Track full scan retry attempts
    const int maxRetries = 4;  // Increased from 3 to 4
    const unsigned long timeout = 3000;  // Reduced from 5000ms to 3000ms
    const unsigned long retryDelay = 300;  // Reduced from 1000ms to 300ms
    
    while (WiFi.status() != WL_CONNECTED && fullScanRetries < maxRetries) {
        if (millis() - startTime > timeout) {
            LogBox::line("Connection timeout, retrying...");
            WiFi.disconnect();
            delay(retryDelay);
            WiFi.begin(ssid.c_str(), password.c_str());
            startTime = millis();
            fullScanRetries++;
        }
        delay(10);  // Reduced polling interval
    }
    
    // Calculate total retry count:
    // - If channel lock was used and failed: 1 for the fallback + full scan timeout retries
    // - If no channel lock: only full scan timeout retries
    // fullScanRetries is incremented each time we timeout in the full scan loop
    if (useChannelLock) {
        retryCount = 1 + fullScanRetries;  // Channel lock fallback (1) + full scan timeouts
    } else {
        retryCount = fullScanRetries;  // Only full scan timeout retries
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.setSleep(false);
        LogBox::line("Connected to WiFi!");
        LogBox::line("IP Address: " + WiFi.localIP().toString());
        LogBox::linef("Signal Strength: %d dBm", WiFi.RSSI());
        
        // Save channel and BSSID for future fast connections
        if (shouldSaveChannel && _configManager) {
            uint8_t channel = WiFi.channel();
            uint8_t* bssid = WiFi.BSSID();
            if (channel > 0 && bssid != nullptr) {
                _configManager->setWiFiChannelLock(channel, bssid);
                LogBox::line("Saved channel/BSSID for future fast connects");
            }
        }
        
        LogBox::end();
        if (outRetryCount) *outRetryCount = retryCount;
        return true;
    } else {
        LogBox::linef("Failed to connect to WiFi after %d retries", maxRetries);
        LogBox::end();
        if (outRetryCount) *outRetryCount = retryCount;
        return false;
    }
}

bool WiFiManager::connectToWiFi(uint8_t* outRetryCount) {
    if (!_configManager) {
        LogBox::message("WiFi Connection", "ConfigManager not set");
        if (outRetryCount) *outRetryCount = 0;
        return false;
    }
    
    String ssid = _configManager->getWiFiSSID();
    String password = _configManager->getWiFiPassword();
    
    if (ssid.length() == 0) {
        LogBox::message("WiFi Connection", "No WiFi credentials stored");
        if (outRetryCount) *outRetryCount = 0;
        return false;
    }
    
    return connectToWiFi(ssid, password, outRetryCount);
}

void WiFiManager::disconnect() {
    if (WiFi.status() == WL_CONNECTED) {
        LogBox::message("WiFi", "Disconnecting from WiFi...");
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

bool WiFiManager::configureStaticIP(const String& ip, const String& gateway, const String& subnet, 
                                    const String& dns1, const String& dns2) {
    LogBox::line("Network mode: Static IP");
    
    // Parse IP addresses
    IPAddress ipAddr, gatewayAddr, subnetAddr, dns1Addr, dns2Addr;
    
    if (!ipAddr.fromString(ip)) {
        LogBox::line("ERROR: Invalid static IP address: " + ip);
        return false;
    }
    
    if (!gatewayAddr.fromString(gateway)) {
        LogBox::line("ERROR: Invalid gateway address: " + gateway);
        return false;
    }
    
    if (!subnetAddr.fromString(subnet)) {
        LogBox::line("ERROR: Invalid subnet mask: " + subnet);
        return false;
    }
    
    if (!dns1Addr.fromString(dns1)) {
        LogBox::line("ERROR: Invalid primary DNS: " + dns1);
        return false;
    }
    
    // Secondary DNS is optional
    if (dns2.length() > 0) {
        if (!dns2Addr.fromString(dns2)) {
            LogBox::line("WARNING: Invalid secondary DNS: " + dns2 + ", ignoring");
            dns2Addr = IPAddress(0, 0, 0, 0);
        }
    } else {
        dns2Addr = IPAddress(0, 0, 0, 0);
    }
    
    // Configure static IP
    if (!WiFi.config(ipAddr, gatewayAddr, subnetAddr, dns1Addr, dns2Addr)) {
        LogBox::line("ERROR: Failed to configure static IP");
        return false;
    }
    
    LogBox::line("Static IP: " + ip);
    LogBox::line("Gateway: " + gateway);
    LogBox::line("Subnet: " + subnet);
    LogBox::line("Primary DNS: " + dns1);
    if (dns2.length() > 0) {
        LogBox::line("Secondary DNS: " + dns2);
    }
    
    return true;
}
