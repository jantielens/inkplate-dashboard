#include "image_manager.h"
#include "logger.h"
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
    LogBox::line(message);
}

void ImageManager::showError(const char* error) {
    _lastError = error;
    LogBox::line("Image Error: " + _lastError);
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
        LogBox::line("Warning: CRC32 parsing may be incomplete");
    }
    
    return result;
}

bool ImageManager::checkCRC32Changed(const char* url) {
    if (!_configManager) {
        LogBox::begin("CRC32 Check");
        LogBox::line("ConfigManager not set - cannot check CRC32");
        LogBox::end();
        return true;  // Fallback to download
    }
    
    LogBox::begin("Checking CRC32 for changes");
    
    // Construct CRC32 URL
    String crc32Url = String(url) + ".crc32";
    LogBox::line("CRC32 URL: " + crc32Url);
    
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
        LogBox::linef("CRC32 file not found or error (code: %d)", httpCode);
        LogBox::line("Falling back to full image download");
        LogBox::end();
        http.end();
        return true;  // Fallback to download
    }
    
    // Read CRC32 content
    String crc32Content = http.getString();
    http.end();
    
    if (crc32Content.length() == 0) {
        LogBox::line("CRC32 file is empty");
        LogBox::line("Falling back to full image download");
        LogBox::end();
        return true;  // Fallback to download
    }
    
    LogBox::line("CRC32 file content: " + crc32Content);
    
    // Parse hex CRC32
    uint32_t newCRC32 = parseHexCRC32(crc32Content);
    LogBox::linef("Parsed CRC32: 0x%08X", newCRC32);
    
    // Get stored CRC32
    uint32_t storedCRC32 = _configManager->getLastCRC32();
    LogBox::linef("Stored CRC32: 0x%08X", storedCRC32);
    
    // Compare
    if (newCRC32 == storedCRC32 && storedCRC32 != 0) {
        LogBox::end("CRC32 UNCHANGED - Skipping download");
        return false;  // No change, skip download
    } else {
        LogBox::line("CRC32 CHANGED - Will download image");
        
        // Update stored CRC32
        _configManager->setLastCRC32(newCRC32);
        LogBox::end();
        return true;  // Changed, proceed with download
    }
}

bool ImageManager::downloadAndDisplay(const char* url) {
    _lastError = "";
    
    LogBox::begin("Starting image download");
    LogBox::line("URL: " + String(url));
    
    // Determine if HTTPS or HTTP
    bool useHttps = isHttps(url);
    
    HTTPClient http;
    WiFiClient client;
    WiFiClientSecure secureClient;
    
    if (useHttps) {
        LogBox::line("Using HTTPS connection");
        // Don't verify certificate for simplicity (public images)
        secureClient.setInsecure();
        http.begin(secureClient, url);
    } else {
        LogBox::line("Using HTTP connection");
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
        LogBox::end();
        http.end();
        return false;
    }
    
    // Get content length
    int contentLength = http.getSize();
    LogBox::linef("Content-Length: %d bytes", contentLength);
    
    if (contentLength <= 0) {
        showError("Invalid content length");
        LogBox::end();
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
        LogBox::line("Image downloaded and displayed successfully!");
        success = true;
    } else {
        showError("Failed to draw image (invalid format or size mismatch)");
        success = false;
    }
    
    http.end();
    
    if (success) {
        LogBox::end("Image display complete!");
    } else {
        LogBox::end();
    }
    
    return success;
}

const char* ImageManager::getLastError() {
    return _lastError.c_str();
}
