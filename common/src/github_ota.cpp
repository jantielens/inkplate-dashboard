#include "github_ota.h"
#include <Update.h>
#include "logger.h"
#include "version.h"

// Global progress tracking instance
OTAProgress g_otaProgress;

GitHubOTA::GitHubOTA() {
}

GitHubOTA::~GitHubOTA() {
    _http.end();
}

bool GitHubOTA::checkLatestRelease(const String& boardName, ReleaseInfo& info) {
    _lastError = "";
    info.found = false;
    
    // Build API URL for latest release
    String url = String(GITHUB_API_BASE) + "/repos/" + GITHUB_REPO_OWNER + "/" + GITHUB_REPO_NAME + "/releases/latest";
    
    Logger::begin("GitHub OTA");
    Logger::line("Checking for updates...");
    Logger::line("URL: " + url);
    Logger::end();
    
    // Configure HTTP client
    _http.begin(url);
    _http.addHeader("Accept", "application/vnd.github.v3+json");
    _http.addHeader("User-Agent", "Inkplate-Dashboard-OTA");
    _http.setTimeout(30000); // 30 second timeout
    
    // Make API request
    int httpCode = _http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        if (httpCode == 403) {
            _lastError = "GitHub API rate limit exceeded. Please try again later.";
        } else if (httpCode == 404) {
            _lastError = "No releases found in repository.";
        } else if (httpCode < 0) {
            _lastError = "Network error: " + _http.errorToString(httpCode);
        } else {
            _lastError = "HTTP error: " + String(httpCode);
        }
        
        Logger::message("GitHub OTA Error", _lastError);
        
        _http.end();
        return false;
    }
    
    // Parse JSON response
    String payload = _http.getString();
    _http.end();
    
    // Use JsonDocument with dynamic allocation
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        _lastError = "Failed to parse JSON: " + String(error.c_str());
        Logger::message("GitHub OTA Error", _lastError);
        return false;
    }
    
    // Extract release information
    info.tagName = doc["tag_name"].as<String>();
    info.publishedAt = doc["published_at"].as<String>();
    
    // Remove 'v' prefix from version if present
    info.version = info.tagName;
    if (info.version.startsWith("v")) {
        info.version = info.version.substring(1);
    }
    
    Logger::begin("GitHub Release");
    Logger::line("Latest: " + info.tagName);
    
    // Find matching asset for this board
    String assetPrefix = boardNameToAssetPrefix(boardName);
    JsonArray assets = doc["assets"];
    
    for (JsonObject asset : assets) {
        String assetName = asset["name"].as<String>();
        
        // Match pattern: {board}-v{version}.bin
        // Example: inkplate2-v0.15.0.bin
        if (assetName.startsWith(assetPrefix + "-v") && assetName.endsWith(".bin")) {
            // Exclude bootloader and partitions files
            if (assetName.indexOf(".bootloader.") == -1 && assetName.indexOf(".partitions.") == -1) {
                info.assetName = assetName;
                info.assetUrl = asset["browser_download_url"].as<String>();
                info.assetSize = asset["size"].as<size_t>();
                info.found = true;
                
                Logger::linef("Asset: %s (%d KB)", info.assetName.c_str(), info.assetSize / 1024);
                Logger::end();
                
                break;
            }
        }
    }
    
    if (!info.found) {
        _lastError = "No firmware asset found for board: " + boardName + " (looking for: " + assetPrefix + "-v*.bin)";
        Logger::message("GitHub OTA Error", _lastError);
        return false;
    }
    
    return true;
}

bool GitHubOTA::downloadAndInstall(const String& assetUrl, ProgressCallback progressCallback) {
    _lastError = "";
    
    // Initialize progress tracking
    g_otaProgress.inProgress = true;
    g_otaProgress.bytesDownloaded = 0;
    g_otaProgress.totalBytes = 0;
    g_otaProgress.percentComplete = 0;
    
    Logger::begin("GitHub OTA");
    Logger::line("Starting download...");
    Logger::line("URL: " + assetUrl);
    Logger::end();
    
    // Configure HTTP client for download
    _http.begin(assetUrl);
    _http.setTimeout(300000); // 5 minute timeout for download
    _http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); // Follow GitHub redirects
    
    // Start HTTP request
    int httpCode = _http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        _lastError = "Download failed: HTTP " + String(httpCode);
        Logger::message("GitHub OTA Error", _lastError);
        _http.end();
        g_otaProgress.inProgress = false;
        return false;
    }
    
    // Get content length
    int contentLength = _http.getSize();
    
    if (contentLength <= 0) {
        _lastError = "Invalid content length";
        Logger::message("GitHub OTA Error", _lastError);
        _http.end();
        g_otaProgress.inProgress = false;
        return false;
    }
    
    g_otaProgress.totalBytes = contentLength;
    
    Logger::begin("GitHub OTA");
    Logger::linef("Size: %d KB", contentLength / 1024);
    
    // Begin OTA update
    if (!Update.begin(contentLength)) {
        _lastError = "Not enough space for OTA update";
        Logger::end("ERROR: " + _lastError);
        _http.end();
        g_otaProgress.inProgress = false;
        return false;
    }
    
    // Get stream
    WiFiClient* stream = _http.getStreamPtr();
    
    // Download and write firmware in chunks
    size_t written = 0;
    uint8_t buffer[4096];
    
    Logger::line("Writing firmware...");
    
    while (_http.connected() && (written < contentLength)) {
        // Get available data size
        size_t available = stream->available();
        
        if (available) {
            // Read up to buffer size
            size_t toRead = min(available, sizeof(buffer));
            size_t bytesRead = stream->readBytes(buffer, toRead);
            
            // Write to update partition
            size_t bytesWritten = Update.write(buffer, bytesRead);
            
            if (bytesWritten != bytesRead) {
                _lastError = "Write error during OTA update";
                Logger::message("GitHub OTA Error", _lastError);
                Update.abort();
                _http.end();
                return false;
            }
            
            written += bytesWritten;
            
            // Update global progress
            g_otaProgress.bytesDownloaded = written;
            g_otaProgress.percentComplete = (written * 100) / contentLength;
            
            // Call progress callback if provided
            if (progressCallback) {
                progressCallback(written, contentLength);
            }
            
            // Log progress every 100KB
            static size_t lastLoggedKB = 0;
            size_t currentKB = written / 1024;
            if (currentKB - lastLoggedKB >= 100) {
                Logger::messagef("GitHub OTA Progress", "%d KB / %d KB (%d%%)", 
                    currentKB, 
                    contentLength / 1024, 
                    (written * 100) / contentLength);
                lastLoggedKB = currentKB;
            }
        }
        
        delay(1);
    }
    
    _http.end();
    
    // Verify download completed
    if (written != contentLength) {
        _lastError = "Download incomplete: " + String(written) + " / " + String(contentLength);
        Logger::message("GitHub OTA Error", _lastError);
        Update.abort();
        g_otaProgress.inProgress = false;
        return false;
    }
    
    // Finalize update
    if (!Update.end(true)) {
        _lastError = "Update finalization failed: " + String(Update.getError());
        Logger::message("GitHub OTA Error", _lastError);
        g_otaProgress.inProgress = false;
        return false;
    }
    
    Logger::begin("GitHub OTA");
    Logger::line("✓ Firmware update successful!");
    Logger::line("Device will reboot...");
    Logger::end();
    
    g_otaProgress.inProgress = false;
    g_otaProgress.percentComplete = 100;
    
    return true;
}

bool GitHubOTA::isNewerVersion(const String& current, const String& latest) {
    int currentMajor, currentMinor, currentPatch;
    int latestMajor, latestMinor, latestPatch;
    
    if (!parseVersion(current, currentMajor, currentMinor, currentPatch)) {
        return false;
    }
    
    if (!parseVersion(latest, latestMajor, latestMinor, latestPatch)) {
        return false;
    }
    
    // Compare versions
    if (latestMajor > currentMajor) return true;
    if (latestMajor < currentMajor) return false;
    
    if (latestMinor > currentMinor) return true;
    if (latestMinor < currentMinor) return false;
    
    if (latestPatch > currentPatch) return true;
    
    return false;
}

String GitHubOTA::boardNameToAssetPrefix(const String& boardName) {
    // Convert board name to lowercase asset prefix
    // "Inkplate 2" -> "inkplate2"
    // "Inkplate 5 V2" -> "inkplate5v2"
    // "Inkplate 10" -> "inkplate10"
    // "Inkplate 6 Flick" -> "inkplate6flick"
    
    String prefix = boardName;
    prefix.toLowerCase();
    prefix.replace(" ", "");
    
    return prefix;
}

bool GitHubOTA::parseVersion(const String& version, int& major, int& minor, int& patch) {
    // Remove 'v' or 'V' prefix if present
    String cleanVersion = version;
    if (cleanVersion.startsWith("v") || cleanVersion.startsWith("V")) {
        cleanVersion = cleanVersion.substring(1);
    }
    
    // Parse version string "major.minor.patch"
    int firstDot = cleanVersion.indexOf('.');
    if (firstDot == -1) return false;
    
    int secondDot = cleanVersion.indexOf('.', firstDot + 1);
    if (secondDot == -1) return false;
    
    major = cleanVersion.substring(0, firstDot).toInt();
    minor = cleanVersion.substring(firstDot + 1, secondDot).toInt();
    patch = cleanVersion.substring(secondDot + 1).toInt();
    
    return true;
}
