#include "image_manager.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

ImageManager::ImageManager(Inkplate* display, DisplayManager* displayManager) {
    _display = display;
    _displayManager = displayManager;
    _configManager = nullptr;
    _lastError = "";
}

void ImageManager::setConfigManager(ConfigManager* configManager) {
    _configManager = configManager;
}

bool ImageManager::isHttps(const char* url) {
    return (strncmp(url, "https://", 8) == 0);
}

void ImageManager::showDownloadProgress(const char* message) {
    Serial.println(message);
}

void ImageManager::showError(const char* error) {
    _lastError = error;
    Serial.println("Image Error: " + _lastError);
}

uint32_t ImageManager::parseHexCRC32(const String& hexStr) {
    // Parse hex string to uint32_t
    // Expected format: "0x12345678" or "12345678"
    String cleaned = hexStr;
    cleaned.trim();
    
    // Remove "0x" prefix if present
    if (cleaned.startsWith("0x") || cleaned.startsWith("0X")) {
        cleaned = cleaned.substring(2);
    }
    
    // Parse as hex
    char* endptr;
    uint32_t result = strtoul(cleaned.c_str(), &endptr, 16);
    
    // Check if parsing was successful
    if (*endptr != '\0' && *endptr != '\n' && *endptr != '\r') {
        Serial.println("Warning: CRC32 parsing may be incomplete");
    }
    
    return result;
}

bool ImageManager::checkCRC32Changed(const char* url) {
    if (!_configManager) {
        Serial.println("ConfigManager not set - cannot check CRC32");
        return true;  // Fallback to download
    }
    
    Serial.println("\n=================================");
    Serial.println("Checking CRC32 for changes...");
    Serial.println("=================================\n");
    
    // Construct CRC32 URL
    String crc32Url = String(url) + ".crc32";
    Serial.println("CRC32 URL: " + crc32Url);
    
    // Determine if HTTPS or HTTP
    bool useHttps = isHttps(crc32Url.c_str());
    
    HTTPClient http;
    WiFiClient client;
    WiFiClientSecure secureClient;
    
    if (useHttps) {
        secureClient.setInsecure();
        http.begin(secureClient, crc32Url);
    } else {
        http.begin(client, crc32Url);
    }
    
    // Set timeout (shorter for CRC32 file)
    http.setTimeout(10000);  // 10 seconds
    http.setUserAgent("InkplateDashboard/1.0");
    
    // Send GET request
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.println("CRC32 file not found or error (code: " + String(httpCode) + ")");
        Serial.println("Falling back to full image download");
        http.end();
        return true;  // Fallback to download
    }
    
    // Read CRC32 content
    String crc32Content = http.getString();
    http.end();
    
    if (crc32Content.length() == 0) {
        Serial.println("CRC32 file is empty");
        Serial.println("Falling back to full image download");
        return true;  // Fallback to download
    }
    
    Serial.println("CRC32 file content: " + crc32Content);
    
    // Parse hex CRC32
    uint32_t newCRC32 = parseHexCRC32(crc32Content);
    Serial.println("Parsed CRC32: 0x" + String(newCRC32, HEX));
    
    // Get stored CRC32
    uint32_t storedCRC32 = _configManager->getLastCRC32();
    Serial.println("Stored CRC32: 0x" + String(storedCRC32, HEX));
    
    // Compare
    if (newCRC32 == storedCRC32 && storedCRC32 != 0) {
        Serial.println("\n=================================");
        Serial.println("CRC32 UNCHANGED - Skipping download");
        Serial.println("=================================\n");
        return false;  // No change, skip download
    } else {
        Serial.println("\n=================================");
        Serial.println("CRC32 CHANGED - Will download image");
        Serial.println("=================================\n");
        
        // Update stored CRC32
        _configManager->setLastCRC32(newCRC32);
        return true;  // Changed, proceed with download
    }
}

bool ImageManager::downloadAndDisplay(const char* url) {
    _lastError = "";
    
    Serial.println("\n=================================");
    Serial.println("Starting image download...");
    Serial.println("URL: " + String(url));
    Serial.println("=================================\n");
    
    // Determine if HTTPS or HTTP
    bool useHttps = isHttps(url);
    
    HTTPClient http;
    WiFiClient client;
    WiFiClientSecure secureClient;
    
    if (useHttps) {
        Serial.println("Using HTTPS connection");
        // Don't verify certificate for simplicity (public images)
        secureClient.setInsecure();
        http.begin(secureClient, url);
    } else {
        Serial.println("Using HTTP connection");
        http.begin(client, url);
    }
    
    // Set timeout
    http.setTimeout(30000);  // 30 seconds
    
    // Set user agent
    http.setUserAgent("InkplateDashboard/1.0");
    
    // Send GET request
    showDownloadProgress("Sending HTTP request...");
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        showError(("HTTP error: " + String(httpCode)).c_str());
        http.end();
        return false;
    }
    
    // Get content length
    int contentLength = http.getSize();
    Serial.println("Content-Length: " + String(contentLength) + " bytes");
    
    if (contentLength <= 0) {
        showError("Invalid content length");
        http.end();
        return false;
    }
    
    showDownloadProgress("Downloading image data...");
    
    // Use the InkPlate library's drawImage function
    // The library supports drawing PNG directly from a stream
    bool success = false;
    
    // Try to draw the image from the web stream
    // InkPlate library has drawImage that can work with WiFiClient streams
    if (_display->drawImage(url, 0, 0, true, false)) {
        Serial.println("Image downloaded and displayed successfully!");
        success = true;
    } else {
        showError("Failed to draw image (invalid format or size mismatch)");
        success = false;
    }
    
    http.end();
    
    if (success) {
        Serial.println("\n=================================");
        Serial.println("Image display complete!");
        Serial.println("=================================\n");
    }
    
    return success;
}

const char* ImageManager::getLastError() {
    return _lastError.c_str();
}
