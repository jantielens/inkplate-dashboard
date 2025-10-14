#ifndef IMAGE_MANAGER_H
#define IMAGE_MANAGER_H

#include "Inkplate.h"
#include "display_manager.h"

class ImageManager {
public:
    ImageManager(Inkplate* display, DisplayManager* displayManager);
    
    // Download and display image from URL
    bool downloadAndDisplay(const char* url);
    
    // Get last error message
    const char* getLastError();
    
private:
    Inkplate* _display;
    DisplayManager* _displayManager;
    String _lastError;
    
    // Helper functions
    bool isHttps(const char* url);
    void showDownloadProgress(const char* message);
    void showError(const char* error);
};

#endif // IMAGE_MANAGER_H
