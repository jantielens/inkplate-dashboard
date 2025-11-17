#include "ui/screen.h"
#include "logo_bitmap.h"
#include "config.h"

Screen::Screen(DisplayManager* displayManager, OverlayManager* overlayMgr, float batteryVoltage) : UIBase(displayManager),
    _showLogo(true),  // Logo enabled by default
    _showBattery(overlayMgr != nullptr && batteryVoltage > 0.0f),  // Battery enabled if overlayManager provided and voltage valid
    _enableRotation(true),  // Default: rotation enabled
    _currentY(MARGIN),
    _batteryVoltage(batteryVoltage) {
    // Set overlay manager for battery icon support
    if (overlayMgr != nullptr) {
        setOverlayManager(overlayMgr);
    }
    // Enable rotation by default (can be overridden with withoutRotation())
    displayManager->enableRotation();
    // Clear screen at construction so content can be added immediately
    displayManager->clear();
    
    // Draw logo immediately if enabled (updates _currentY)
    if (_showLogo) {
        drawLogo();
    }
}

Screen::~Screen() {
}

Screen& Screen::withoutLogo() {
    // Note: Logo already drawn in constructor if it was enabled
    // This method is mainly useful if called before constructor completes
    // or to document intent that logo should not be shown
    _showLogo = false;
    return *this;
}

Screen& Screen::withoutBattery() {
    _showBattery = false;
    return *this;
}

Screen& Screen::withRotation() {
    _enableRotation = true;
    return *this;
}

Screen& Screen::withoutRotation() {
    _enableRotation = false;
    displayManager->disableRotation();
    return *this;
}

Screen& Screen::addHeading1(const String& text) {
    ensureLineHeight(nullptr);  // Will use displayManager's current font height
    displayManager->showMessage(text.c_str(), MARGIN, _currentY, FONT_HEADING1);
    _currentY += displayManager->getFontHeight(FONT_HEADING1) + LINE_SPACING;
    return *this;
}

Screen& Screen::addHeading2(const String& text) {
    ensureLineHeight(nullptr);
    displayManager->showMessage(text.c_str(), MARGIN, _currentY, FONT_HEADING2);
    _currentY += displayManager->getFontHeight(FONT_HEADING2) + LINE_SPACING;
    return *this;
}

Screen& Screen::addText(const String& text) {
    ensureLineHeight(nullptr);
    displayManager->showMessage(text.c_str(), MARGIN, _currentY, FONT_NORMAL);
    _currentY += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    return *this;
}

Screen& Screen::addSpacing(int pixels) {
    _currentY += pixels;
    return *this;
}

Screen& Screen::addKeyValue(const String& key, const String& value) {
    ensureLineHeight(nullptr);
    String combined = key + ": " + value;
    displayManager->showMessage(combined.c_str(), MARGIN, _currentY, FONT_NORMAL);
    _currentY += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    return *this;
}

Screen& Screen::addNumberedItem(uint8_t number, const String& text) {
    ensureLineHeight(nullptr);
    String combined = String(number) + ". " + text;
    displayManager->showMessage(combined.c_str(), MARGIN, _currentY, FONT_NORMAL);
    _currentY += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    return *this;
}

void Screen::display() {
    // Draw battery icon if requested (call BEFORE refresh for proper rendering)
    if (_showBattery) {
        drawBattery();
    }
    
    // Refresh display
    displayManager->refresh();
}

void Screen::drawLogo() {
    // Center logo horizontally
    int screenWidth = displayManager->getWidth();
    int minLogoX = MARGIN;
    int maxLogoX = screenWidth - LOGO_WIDTH - MARGIN;
    int logoX;
    if (maxLogoX <= minLogoX) {
        logoX = minLogoX;
    } else {
        logoX = minLogoX + (maxLogoX - minLogoX) / 2;
    }
    
    // Position logo vertically - always at top
    int logoY = MARGIN;
    
    // Draw logo bitmap
    displayManager->drawBitmap(logo_bitmap, logoX, logoY, LOGO_WIDTH, LOGO_HEIGHT);
    
    // Update currentY to position content below logo
    _currentY = logoY + LOGO_HEIGHT + MARGIN;
}

void Screen::drawBattery() {
    // Use the battery voltage stored via withBattery()
    drawBatteryIconBottomLeft(_batteryVoltage);
}

void Screen::ensureLineHeight(const GFXfont* font) {
    // Optional: Could add logic here to ensure minimum spacing
    // or auto-adjust Y position if content exceeds screen bounds
    // For now, just a placeholder for future enhancements
}
