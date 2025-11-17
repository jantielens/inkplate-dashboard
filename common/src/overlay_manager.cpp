#include "overlay_manager.h"
#include "battery_logic.h"
#include "board_config.h"
#include "logger.h"
#include <src/fonts/FreeSans7pt7b.h>
#include <src/fonts/Roboto_Regular12pt7b.h>
#include <src/fonts/Roboto_Bold20pt7b.h>

OverlayManager::OverlayManager(Inkplate* display, DisplayManager* displayManager) {
    _display = display;
    _displayManager = displayManager;
}

const GFXfont* OverlayManager::getFontForSize(uint8_t size) {
    switch (size) {
        case OVERLAY_SIZE_SMALL:
            return &FreeSans7pt7b;
        case OVERLAY_SIZE_LARGE:
            return &Roboto_Bold20pt7b;
        case OVERLAY_SIZE_MEDIUM:
        default:
            return &Roboto_Regular12pt7b;
    }
}

void OverlayManager::drawBatteryIcon(int x, int y, int width, int height, int percentage, int color) {
    // Clamp percentage to valid range
    if (percentage < 0) percentage = 0;
    if (percentage > 100) percentage = 100;
    
    // Battery body dimensions
    int bodyWidth = width - 2;  // Leave space for terminal
    int bodyHeight = height;
    int bodyX = x;
    int bodyY = y;
    
    // Battery terminal (positive tip) - small rectangle on the right
    int terminalWidth = 2;
    int terminalHeight = height / 3;
    int terminalX = x + bodyWidth;
    int terminalY = y + (height - terminalHeight) / 2;
    
    // Draw battery outline (body)
    _display->drawRect(bodyX, bodyY, bodyWidth, bodyHeight, color);
    
    // Draw battery terminal
    _display->fillRect(terminalX, terminalY, terminalWidth, terminalHeight, color);
    
    // Calculate fill level (with 1px padding inside)
    int fillPadding = 2;
    int fillableWidth = bodyWidth - (fillPadding * 2);
    int fillableHeight = bodyHeight - (fillPadding * 2);
    int fillWidth = (fillableWidth * percentage) / 100;
    
    // Draw battery fill level
    if (fillWidth > 0) {
        _display->fillRect(bodyX + fillPadding, 
                          bodyY + fillPadding, 
                          fillWidth, 
                          fillableHeight, 
                          color);
    }
}

void OverlayManager::calculateOverlayPosition(const DashboardConfig& config,
                                             int overlayWidth, int overlayHeight,
                                             int* outX, int* outY) {
    int margin = MARGIN;  // Use board-defined margin
    int screenWidth = _displayManager->getWidth();
    int screenHeight = _displayManager->getHeight();
    
    switch (config.overlayPosition) {
        case OVERLAY_POS_TOP_LEFT:
            *outX = margin;
            *outY = margin;
            break;
            
        case OVERLAY_POS_TOP_RIGHT:
            *outX = screenWidth - overlayWidth - margin;
            *outY = margin;
            break;
            
        case OVERLAY_POS_BOTTOM_LEFT:
            *outX = margin;
            *outY = screenHeight - overlayHeight - margin;
            break;
            
        case OVERLAY_POS_BOTTOM_RIGHT:
            *outX = screenWidth - overlayWidth - margin;
            *outY = screenHeight - overlayHeight - margin;
            break;
            
        default:
            // Default to top-right
            *outX = screenWidth - overlayWidth - margin;
            *outY = margin;
            break;
    }
}

void OverlayManager::renderOverlay(const DashboardConfig& config,
                                  float batteryVoltage,
                                  const char* updateTimeStr,
                                  unsigned long cycleTimeMs) {
    // If overlay is disabled, do nothing
    if (!config.overlayEnabled) {
        return;
    }
    
    Logger::begin("Rendering Overlay");
    
    // Select font based on size
    const GFXfont* font = getFontForSize(config.overlaySize);
    int fontHeight = _displayManager->getFontHeight(font);
    
    // Determine text color
    int textColor = (config.overlayTextColor == OVERLAY_COLOR_WHITE) ? WHITE : BLACK;
    
    // Build overlay text string
    String overlayText = "";
    bool hasBattery = (batteryVoltage > 0.0);
    int batteryPercentage = 0;
    
    if (hasBattery && (config.overlayShowBatteryIcon || config.overlayShowBatteryPercentage)) {
        batteryPercentage = calculateBatteryPercentage(batteryVoltage);
        
        if (config.overlayShowBatteryPercentage) {
            overlayText += String(batteryPercentage) + "%";
        }
    }
    
    if (config.overlayShowUpdateTime && updateTimeStr != nullptr && strlen(updateTimeStr) > 0) {
        if (overlayText.length() > 0) overlayText += " ";
        overlayText += String(updateTimeStr);
    }
    
    if (config.overlayShowCycleTime && cycleTimeMs > 0) {
        if (overlayText.length() > 0) overlayText += " ";
        // Convert ms to seconds
        float cycleTimeSec = cycleTimeMs / 1000.0;
        overlayText += String(cycleTimeSec, 1) + "s";
    }
    
    // Calculate text bounds
    _display->setFont(font);
    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    _display->getTextBounds(overlayText.c_str(), 0, 0, &x1, &y1, &textWidth, &textHeight);
    
    // Calculate battery icon dimensions based on font height
    int iconWidth = 0;
    int iconHeight = 0;
    int iconPadding = 4;  // Padding between icon and text
    
    if (hasBattery && config.overlayShowBatteryIcon) {
        iconHeight = fontHeight - 4;  // Slightly smaller than font height
        iconWidth = (iconHeight * 5) / 3;  // Maintain ~3:5 aspect ratio for battery
    }
    
    // Calculate total overlay dimensions
    int totalWidth = textWidth;
    if (iconWidth > 0) {
        totalWidth += iconWidth + iconPadding;
    }
    int totalHeight = fontHeight;
    
    // Calculate overlay position
    int overlayX, overlayY;
    calculateOverlayPosition(config, totalWidth, totalHeight, &overlayX, &overlayY);
    
    Logger::linef("Position: %d,%d Size: %dx%d", overlayX, overlayY, totalWidth, totalHeight);
    Logger::linef("Text: %s", overlayText.c_str());
    
    // Draw battery icon if enabled
    int currentX = overlayX;
    if (iconWidth > 0) {
        int iconY = overlayY + (totalHeight - iconHeight) / 2;
        drawBatteryIcon(currentX, iconY, iconWidth, iconHeight, batteryPercentage, textColor);
        currentX += iconWidth + iconPadding;
    }
    
    // Draw text
    if (overlayText.length() > 0) {
        _display->setFont(font);
        _display->setTextColor(textColor);
        
        // Calculate baseline Y position (GFXfonts use baseline positioning)
        // y1 is negative and represents distance from baseline to top
        int baselineY = overlayY - y1;
        
        _display->setCursor(currentX, baselineY);
        _display->print(overlayText);
    }
    
    Logger::end();
}
