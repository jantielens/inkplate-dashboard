#include "display_manager.h"

DisplayManager::DisplayManager(Inkplate* display) {
    _display = display;
}

void DisplayManager::init() {
    _display->begin();
    _display->clearDisplay();
}

void DisplayManager::clear() {
    _display->clearDisplay();
}

void DisplayManager::refresh() {
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
