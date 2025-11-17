#include <src/ui/ui_base.h>
#include <src/battery_logic.h>
#include <src/config.h>

UIBase::UIBase(DisplayManager* display) 
    : displayManager(display), overlayManager(nullptr) {
}

void UIBase::setOverlayManager(OverlayManager* overlayMgr) {
    overlayManager = overlayMgr;
}

void UIBase::drawBatteryIconBottomLeft(float batteryVoltage) {
    if (overlayManager == nullptr || batteryVoltage <= 0.0) {
        return;
    }
    
    // Medium size icon (same as OVERLAY_SIZE_MEDIUM)
    const GFXfont* font = &Roboto_Regular12pt7b;
    int fontHeight = displayManager->getFontHeight(font);
    int iconHeight = fontHeight - 4;
    int iconWidth = (iconHeight * 5) / 3;
    
    int batteryPercentage = calculateBatteryPercentage(batteryVoltage);
    
    // Bottom left position
    int x = MARGIN;
    int y = displayManager->getHeight() - iconHeight - MARGIN;
    
    // Black color (grayscale 0)
    overlayManager->drawBatteryIcon(x, y, iconWidth, iconHeight, batteryPercentage, 0);
}
