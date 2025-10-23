#ifndef GITHUB_OTA_H
#define GITHUB_OTA_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// GitHub repository configuration
#define GITHUB_REPO_OWNER "jantielens"
#define GITHUB_REPO_NAME "inkplate-dashboard"

// API endpoints
#define GITHUB_API_BASE "https://api.github.com"

// Progress callback type
typedef void (*ProgressCallback)(size_t current, size_t total);

// Global progress tracking for OTA updates
struct OTAProgress {
    volatile bool inProgress;
    volatile size_t bytesDownloaded;
    volatile size_t totalBytes;
    volatile int percentComplete;
    
    OTAProgress() : inProgress(false), bytesDownloaded(0), totalBytes(0), percentComplete(0) {}
};

extern OTAProgress g_otaProgress;

/**
 * @brief GitHub OTA Update Client
 * 
 * Handles communication with GitHub Releases API to check for and download
 * firmware updates. Uses unauthenticated API calls (60 requests/hour limit).
 */
class GitHubOTA {
public:
    /**
     * @brief Release information structure
     */
    struct ReleaseInfo {
        String tagName;           // e.g., "v0.15.0"
        String version;           // e.g., "0.15.0" (without 'v' prefix)
        String assetName;         // e.g., "inkplate2-v0.15.0.bin"
        String assetUrl;          // Download URL for the asset
        size_t assetSize;         // Size in bytes
        String publishedAt;       // ISO 8601 timestamp
        bool found;               // Whether a matching asset was found
        
        ReleaseInfo() : assetSize(0), found(false) {}
    };

    GitHubOTA();
    ~GitHubOTA();
    
    /**
     * @brief Check for the latest release on GitHub
     * @param boardName The board identifier (e.g., "Inkplate 2")
     * @param info Output parameter to store release information
     * @return true if API call succeeded (doesn't mean update available)
     */
    bool checkLatestRelease(const String& boardName, ReleaseInfo& info);
    
    /**
     * @brief Download and install firmware from GitHub
     * @param assetUrl The direct download URL from GitHub releases
     * @param progressCallback Optional callback for progress updates
     * @return true if download and flash succeeded
     */
    bool downloadAndInstall(const String& assetUrl, ProgressCallback progressCallback = nullptr);
    
    /**
     * @brief Compare two version strings
     * @param current Current version (e.g., "0.14.0")
     * @param latest Latest version (e.g., "0.15.0")
     * @return true if latest is newer than current
     */
    static bool isNewerVersion(const String& current, const String& latest);
    
    /**
     * @brief Get last error message
     * @return Error description string
     */
    String getLastError() const { return _lastError; }

private:
    HTTPClient _http;
    String _lastError;
    
    /**
     * @brief Convert board name to asset prefix
     * @param boardName Board name from board_config.h (e.g., "Inkplate 2")
     * @return Asset prefix (e.g., "inkplate2")
     */
    String boardNameToAssetPrefix(const String& boardName);
    
    /**
     * @brief Parse version string to numeric components
     * @param version Version string (e.g., "0.14.0" or "v0.14.0")
     * @param major Output parameter for major version
     * @param minor Output parameter for minor version
     * @param patch Output parameter for patch version
     * @return true if parsing succeeded
     */
    static bool parseVersion(const String& version, int& major, int& minor, int& patch);
};

#endif // GITHUB_OTA_H
