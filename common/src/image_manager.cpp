#include "image_manager.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

ImageManager::ImageManager(Inkplate* display, DisplayManager* displayManager) {
    _display = display;
    _displayManager = displayManager;
    _lastError = "";
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
    
    // Get the stream
    WiFiClient* stream;
    if (useHttps) {
        stream = &secureClient;
    } else {
        stream = &client;
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
