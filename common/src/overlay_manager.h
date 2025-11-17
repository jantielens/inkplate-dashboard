#ifndef OVERLAY_MANAGER_H
#define OVERLAY_MANAGER_H

#include "Inkplate.h"
#include "display_manager.h"
#include "config_manager.h"

/**
 * @brief Manages on-image overlay rendering for status information
 * 
 * Renders configurable status overlays (battery, time, etc.) on top of
 * dashboard images after download but before display refresh.
 */
class OverlayManager {
public:
    OverlayManager(Inkplate* display, DisplayManager* displayManager);
    
    /**
     * @brief Render overlay on current display buffer
     * 
     * @param config Dashboard configuration containing overlay settings
     * @param batteryVoltage Battery voltage in volts (0.0 if not available)
     * @param updateTimeStr Last update time string (e.g., "11:25" or empty)
     * @param cycleTimeMs Last cycle/loop time in milliseconds (0 if not tracking)
     */
    void renderOverlay(const DashboardConfig& config, 
                      float batteryVoltage,
                      const char* updateTimeStr = "",
                      unsigned long cycleTimeMs = 0);
    
    /**
     * @brief Draw battery icon at specified position
     * 
     * @param x X coordinate (top-left)
     * @param y Y coordinate (top-left)
     * @param width Icon width in pixels
     * @param height Icon height in pixels
     * @param percentage Battery percentage (0-100)
     * @param color Icon color (grayscale 0-7)
     */
    void drawBatteryIcon(int x, int y, int width, int height, int percentage, int color);
    
private:
    Inkplate* _display;
    DisplayManager* _displayManager;
    
    /**
     * @brief Get font for overlay based on size setting
     */
    const GFXfont* getFontForSize(uint8_t size);
    
    /**
     * @brief Calculate overlay position based on configuration
     * 
     * @param config Dashboard configuration
     * @param overlayWidth Total overlay width
     * @param overlayHeight Total overlay height
     * @param outX Output X coordinate
     * @param outY Output Y coordinate
     */
    void calculateOverlayPosition(const DashboardConfig& config,
                                  int overlayWidth, int overlayHeight,
                                  int* outX, int* outY);
};

#endif // OVERLAY_MANAGER_H
