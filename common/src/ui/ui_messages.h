#ifndef UI_MESSAGES_H
#define UI_MESSAGES_H

#include <Arduino.h>
#include <src/display_manager.h>
#include <src/overlay_manager.h>

/**
 * @brief Centralized UI message rendering utilities
 * 
 * This class provides a consistent interface for displaying various types of
 * messages on the e-ink display, reducing code duplication and ensuring
 * uniform formatting across the application.
 */
class UIMessages {
public:
    UIMessages(DisplayManager* display);
    
    // Set overlay manager for battery icon rendering on logo screens
    void setOverlayManager(OverlayManager* overlayMgr);
    
    // Screen builders - return the next Y position after rendering
    int showHeading(const char* text, int startY, bool clearFirst = false);
    int showSubheading(const char* text, int currentY);
    int showNormalText(const char* text, int currentY, bool indent = false);
    int showTextWithSpacing(const char* text, int currentY, int extraSpacing = 0);
    
    // Composite screens
    void showSplashScreen(const char* boardName, int width, int height, float batteryVoltage = 0.0);
    void showConfigInitError();
    
private:
    DisplayManager* displayManager;
    OverlayManager* overlayManager;
    
    int addLineSpacing(int currentY, int multiplier = 1);
    
    // Helper to draw battery icon at bottom left (for logo screens)
    void drawBatteryIconBottomLeft(float batteryVoltage);
};

#endif // UI_MESSAGES_H
