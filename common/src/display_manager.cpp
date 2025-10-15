#include "display_manager.h"
#include <src/version.h>

DisplayManager::DisplayManager(Inkplate* display) {
    _display = display;
}

void DisplayManager::init(bool clearOnInit) {
    _display->begin();
    if (clearOnInit) {
        _display->clearDisplay();
    }
}

void DisplayManager::clear() {
    _display->clearDisplay();
}

void DisplayManager::refresh(bool includeVersion) {
    if (includeVersion) {
        drawVersionLabel();
    }
    _display->display();
}

void DisplayManager::showMessage(const char* message, int x, int y, int textSize) {
    _display->setCursor(x, y);
    _display->setTextSize(textSize);
    _display->setTextColor(BLACK);
    _display->print(message);
}

void DisplayManager::drawCentered(const char* message, int y, int textSize) {
    _display->setTextSize(textSize);
    _display->setTextColor(BLACK);
    
    // Calculate text width (approximate)
    int16_t x1, y1;
    uint16_t w, h;
    _display->getTextBounds(message, 0, 0, &x1, &y1, &w, &h);
    
    int x = (getWidth() - w) / 2;
    _display->setCursor(x, y);
    _display->print(message);
}

int DisplayManager::getWidth() {
    return _display->width();
}

int DisplayManager::getHeight() {
    return _display->height();
}

void DisplayManager::drawVersionLabel() {
    static const char versionLabel[] = "Firmware " FIRMWARE_VERSION;
    _display->setTextSize(scaleFont(2));  // Scale the version label font
    _display->setTextColor(BLACK);

    int16_t x1 = 0;
    int16_t y1 = 0;
    uint16_t w = 0;
    uint16_t h = 0;
    _display->getTextBounds(versionLabel, 0, 0, &x1, &y1, &w, &h);

    int margin = scaleY(8);  // Scale the margin too
    int x = getWidth() - w - margin;
    int y = getHeight() - h - margin;
    if (x < margin) {
        x = margin;
    }
    if (y < margin) {
        y = margin;
    }

    _display->setCursor(x, y);
    _display->print(versionLabel);
}

// Board-specific scaling helpers
int DisplayManager::scaleFont(int baseSize) {
    #ifdef FONT_SCALE_FACTOR
    int scaled = (int)(baseSize * FONT_SCALE_FACTOR + 0.5);  // Round to nearest
    return scaled < 1 ? 1 : scaled;  // Minimum font size is 1
    #else
    return baseSize;  // No scaling if not defined
    #endif
}

int DisplayManager::scaleY(int baseY) {
    #ifdef LAYOUT_SCALE_FACTOR
    return (int)(baseY * LAYOUT_SCALE_FACTOR + 0.5);  // Round to nearest
    #else
    return baseY;  // No scaling if not defined
    #endif
}
