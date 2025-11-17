#include "overlay_manager.h"
#include "battery_logic.h"
#include "board_config.h"
#include "logger.h"

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
    int bodyWidth = width - 4;  // Leave space for terminal (increased from 2 to 4)
    int bodyHeight = height;
    int bodyX = x;
    int bodyY = y;
    
    // Battery terminal (positive tip) - thicker rectangle on the right
    // Use smaller terminal width for small icons, larger for medium/large
    int terminalWidth = (height < 12) ? 2 : 4;
    int terminalHeight = height / 2;
    int terminalX = x + bodyWidth;
    int terminalY = y + (height - terminalHeight) / 2;
    
    // Use rounded corners for medium and large icons
    bool useRoundedCorners = (height >= 12);
    int cornerRadius = useRoundedCorners ? 4 : 0;  // More pronounced corners
    
    // Draw battery outline (body)
    if (useRoundedCorners) {
        _display->drawRoundRect(bodyX, bodyY, bodyWidth, bodyHeight, cornerRadius, color);
    } else {
        _display->drawRect(bodyX, bodyY, bodyWidth, bodyHeight, color);
    }
    
    // Draw battery terminal
    _display->fillRect(terminalX, terminalY, terminalWidth, terminalHeight, color);
    
    // Calculate fill level (with 1px padding inside)
    int fillPadding = 2;
    int fillableWidth = bodyWidth - (fillPadding * 2);
    int fillableHeight = bodyHeight - (fillPadding * 2);
    int fillWidth = (fillableWidth * percentage) / 100;
    
    // Draw battery fill level
    if (fillWidth > 0) {
        if (useRoundedCorners) {
            // Use rounded rect for fill to match the outline
            _display->fillRoundRect(bodyX + fillPadding, 
                                   bodyY + fillPadding, 
                                   fillWidth, 
                                   fillableHeight, 
                                   cornerRadius - 1,  // Slightly smaller radius for inner fill
                                   color);
        } else {
            _display->fillRect(bodyX + fillPadding, 
                              bodyY + fillPadding, 
                              fillWidth, 
                              fillableHeight, 
                              color);
        }
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
    
    // Determine text color (using display's grayscale values)
    int textColor;
    if (config.overlayTextColor == OVERLAY_COLOR_WHITE) {
        textColor = 7;  // White (lightest gray on 3-bit displays)
    } else if (config.overlayTextColor == OVERLAY_COLOR_LIGHT_GRAY) {
        textColor = 5;  // Light gray (~70%)
    } else if (config.overlayTextColor == OVERLAY_COLOR_DARK_GRAY) {
        textColor = 2;  // Dark gray (~30%)
    } else {
        textColor = 0;  // Black
    }
    
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
    // Use actual text bounding box height instead of fontHeight
    // This accounts for descenders and ensures proper bottom positioning
    int totalHeight = textHeight;
    
    // Calculate overlay position
    int overlayX, overlayY;
    calculateOverlayPosition(config, totalWidth, totalHeight, &overlayX, &overlayY);
    
    Logger::linef("Position: %d,%d Size: %dx%d", overlayX, overlayY, totalWidth, totalHeight);
    Logger::linef("Text: %s", overlayText.c_str());
    
    // Calculate baseline Y position (GFXfonts use baseline positioning)
    // For top positions: overlayY is where we want the top, so baseline is overlayY - y1
    // For bottom positions: overlayY + totalHeight is where we want the bottom
    // y1 is negative and represents distance from baseline to top of tallest character
    // textHeight includes both the height above baseline (-y1) and any descenders below
    int baselineY;
    if (config.overlayPosition == OVERLAY_POS_BOTTOM_LEFT || 
        config.overlayPosition == OVERLAY_POS_BOTTOM_RIGHT) {
        // For bottom positioning: work backwards from desired bottom edge
        // overlayY + totalHeight = bottom edge position
        // Baseline should be at: bottom edge - descenders
        // Since textHeight = -y1 + descenders, descenders = textHeight - (-y1)
        baselineY = overlayY + totalHeight - (textHeight - (-y1));
    } else {
        // For top positioning: overlayY is the top edge
        baselineY = overlayY - y1;
    }
    
    // Draw battery icon if enabled
    int currentX = overlayX;
    if (iconWidth > 0) {
        // Vertically center the icon with the text's visual height (cap height)
        // y1 is negative, so -y1 gives the height above baseline
        // This centers the icon with the main body of the text, ignoring descenders
        int textVisibleHeight = -y1;  // Height of text above baseline (cap height area)
        int iconY = baselineY - textVisibleHeight - ((iconHeight - textVisibleHeight) / 2);
        drawBatteryIcon(currentX, iconY, iconWidth, iconHeight, batteryPercentage, textColor);
        currentX += iconWidth + iconPadding;
    }
    
    // Draw text
    if (overlayText.length() > 0) {
        _display->setFont(font);
        _display->setTextColor(textColor);
        _display->setCursor(currentX, baselineY);
        _display->print(overlayText);
    }
    
    Logger::end();
}
