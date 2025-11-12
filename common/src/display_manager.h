#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "Inkplate.h"

// Include font files (provides GFXfont objects referenced by board_config.h)
#include <src/fonts/FreeSans7pt7b.h>
#include <src/fonts/Roboto_Regular12pt7b.h>
#include <src/fonts/Roboto_Bold20pt7b.h>
#include <src/fonts/Roboto_Bold24pt7b.h>

class DisplayManager {
public:
    DisplayManager(Inkplate* display);
    
    void init(bool clearOnInit = true, uint8_t rotation = 0);
    void clear();
    void refresh(bool includeVersion = true);
    void showMessage(const char* message, int x, int y, const GFXfont* font);
    void drawCentered(const char* message, int y, const GFXfont* font);
    
    // Rotation management (for performance optimization)
    void setRotation(uint8_t rotation);
    uint8_t getRotation() const;
    void enableRotation();   // Restore configured rotation
    void disableRotation();  // Set to 0 for performance
    
    // Helper to calculate font height in pixels for GFXfonts
    int getFontHeight(const GFXfont* font);
    
    // Board-specific adaptations can be added here
    int getWidth();
    int getHeight();
    
    // Draw embedded bitmap image (array) at specified location
    void drawBitmap(const uint8_t* bitmap, int x, int y, int w, int h);
    
    #ifndef DISPLAY_MODE_INKPLATE2
    // VCOM management (not available on Inkplate 2 - no TPS65186 PMIC)
    // Read panel VCOM value (in volts, negative). Returns NAN on error.
    double readPanelVCOM();
    // DANGEROUS: Programs the VCOM value in the TPS65186 EEPROM. Use only if you know what you are doing.
    // vcom: negative voltage in volts (e.g., -1.23). Returns true if successful.
    bool programPanelVCOM(double vcom, String* diagnostics = nullptr);
    // Display VCOM test pattern with grayscale bars
    void showVcomTestPattern();
    #endif
    
private:
    Inkplate* _display;
    uint8_t _configuredRotation = 0;  // The rotation configured by user
    uint8_t _currentRotation = 0;     // Current active rotation
    void drawVersionLabel();
};

#endif // DISPLAY_MANAGER_H
