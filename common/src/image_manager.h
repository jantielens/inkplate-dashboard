#ifndef IMAGE_MANAGER_H
#define IMAGE_MANAGER_H

#include "Inkplate.h"
#include "display_manager.h"
#include "config_manager.h"

class ImageManager {
public:
    ImageManager(Inkplate* display, DisplayManager* displayManager, ConfigManager* configManager);
    
    // Download and display image from URL (with checksum-based skip logic)
    bool downloadAndDisplay(const char* url);
    
    // Check if last update was skipped (image unchanged)
    bool wasUpdateSkipped() const;
    
    // Get last computed checksum
    uint32_t getLastChecksum() const;
    
    // Get last error message
    const char* getLastError();
    
private:
    Inkplate* _display;
    DisplayManager* _displayManager;
    ConfigManager* _configManager;
    String _lastError;
    bool _updateSkipped;
    uint32_t _lastChecksum;
    
    // Helper functions
    bool isHttps(const char* url);
    void showDownloadProgress(const char* message);
    void showError(const char* error);
    
    // Checksum computation
    uint32_t downloadAndComputeChecksum(const char* url);
    uint32_t computeCRC32(const uint8_t* data, size_t length, uint32_t previousCrc32 = 0);
};

#endif // IMAGE_MANAGER_H
