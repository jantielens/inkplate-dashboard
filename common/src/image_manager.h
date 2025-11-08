#ifndef IMAGE_MANAGER_H
#define IMAGE_MANAGER_H

#include "Inkplate.h"
#include "display_manager.h"
#include "config_manager.h"

class ImageManager {
public:
    ImageManager(Inkplate* display, DisplayManager* displayManager);
    
    // Set config manager for CRC32 storage
    void setConfigManager(ConfigManager* configManager);
    
    // Check if image has changed based on CRC32
    // Returns true if changed or check failed (should download)
    // Returns false if unchanged (skip download)
    // If outNewCRC32 is provided, outputs the new CRC32 value fetched from server
    // If outRetryCount is provided, outputs the number of retry attempts made (0-2)
    // Note: Does NOT save the CRC32 - caller must call saveCRC32() after successful display
    bool checkCRC32Changed(const char* url, uint32_t* outNewCRC32 = nullptr, uint8_t* outRetryCount = nullptr);
    
    // Save CRC32 value (deferred until after successful image display)
    void saveCRC32(uint32_t crc32Value);
    
    // Download and display image from URL
    bool downloadAndDisplay(const char* url);
    
    // Get last error message
    const char* getLastError();
    
private:
    Inkplate* _display;
    DisplayManager* _displayManager;
    ConfigManager* _configManager;
    String _lastError;
    
    // Helper functions
    bool isHttps(const char* url);
    void showDownloadProgress(const char* message);
    void showError(const char* error);
    uint32_t parseHexCRC32(const String& hexStr);
};

#endif // IMAGE_MANAGER_H
