#include "board_config.h"
#include "display_manager.h"
#include <src/version.h>

DisplayManager::DisplayManager(Inkplate* display) {
    _display = display;
}

void DisplayManager::init(bool clearOnInit, uint8_t rotation) {
    _display->begin();
    
    // Store configured rotation but don't apply it yet
    // This allows performance optimization where rotation is only enabled when needed
    if (rotation <= 3) {
        _configuredRotation = rotation;
        _currentRotation = 0;  // Start with rotation disabled for performance
    }
    
    if (clearOnInit) {
        _display->clearDisplay();
    }
}

void DisplayManager::setRotation(uint8_t rotation) {
    if (rotation <= 3 && rotation != _currentRotation) {
        _display->setRotation(rotation);
        _currentRotation = rotation;
    }
}

uint8_t DisplayManager::getRotation() const {
    return _currentRotation;
}

void DisplayManager::enableRotation() {
    // Enable the user-configured rotation for full-screen UI elements
    setRotation(_configuredRotation);
}

void DisplayManager::disableRotation() {
    // Disable rotation (set to 0) for performance-critical operations
    setRotation(0);
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
    _display->setTextSize(FONT_NORMAL);
    _display->setTextColor(BLACK);

    int16_t x1 = 0;
    int16_t y1 = 0;
    uint16_t w = 0;
    uint16_t h = 0;
    _display->getTextBounds(versionLabel, 0, 0, &x1, &y1, &w, &h);

    int x = getWidth() - w - MARGIN;
    int y = getHeight() - h - MARGIN;
    if (x < MARGIN) {
        x = MARGIN;
    }
    if (y < MARGIN) {
        y = MARGIN;
    }

    _display->setCursor(x, y);
    _display->print(versionLabel);
}

// Helper to calculate approximate font height in pixels
// Inkplate uses a 5x7 font matrix, scaled by textSize
int DisplayManager::getFontHeight(int textSize) {
    return 8 * textSize;  // 7 pixels + 1 pixel spacing, multiplied by size
}
