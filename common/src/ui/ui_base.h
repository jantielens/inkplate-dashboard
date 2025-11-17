#ifndef UI_BASE_H
#define UI_BASE_H

#include <Arduino.h>
#include <src/display_manager.h>
#include <src/overlay_manager.h>

/**
 * @brief Base class for UI components with shared functionality
 * 
 * Provides common utilities for UI rendering, including battery icon display.
 */
class UIBase {
public:
    UIBase(DisplayManager* display);
    
    // Set overlay manager for battery icon rendering
    void setOverlayManager(OverlayManager* overlayMgr);
    
protected:
    DisplayManager* displayManager;
    OverlayManager* overlayManager;
    
    // Helper to draw battery icon at bottom left (for logo screens)
    void drawBatteryIconBottomLeft(float batteryVoltage);
};

#endif // UI_BASE_H
