#include "frontlight_manager.h"
#include "logger.h"

#if defined(HAS_FRONTLIGHT) && HAS_FRONTLIGHT == true

FrontlightManager::FrontlightManager(Inkplate* display)
    : _display(display)
    , _activated(false)
    , _startTime(0)
    , _currentBrightness(0)
    , _minDurationMs(0)
{
}

void FrontlightManager::turnOn(uint8_t brightness, unsigned long minDurationMs) {
    if (!_display) {
        LogBox::line("FrontlightManager: Display pointer is null");
        return;
    }
    
    // Set brightness to 0 before enabling circuit to prevent flicker
    _display->setFrontlight(0);
    
    // Enable the frontlight circuit
    _display->frontlight(true);
    
    // Set to target brightness
    _display->setFrontlight(brightness);
    
    // Update state
    _activated = true;
    _startTime = millis();
    _currentBrightness = brightness;
    _minDurationMs = minDurationMs;
    
    LogBox::begin("Frontlight");
    LogBox::linef("Frontlight activated (brightness: %d, min duration: %lu ms)", 
                 brightness, minDurationMs);
    LogBox::end();
}

void FrontlightManager::turnOff() {
    if (!_activated || !_display) {
        return;
    }
    
    // Check if minimum duration has elapsed
    if (_minDurationMs > 0) {
        unsigned long elapsedTime = millis() - _startTime;
        if (elapsedTime < _minDurationMs) {
            unsigned long remainingTime = _minDurationMs - elapsedTime;
            LogBox::begin("Frontlight");
            LogBox::linef("Minimum duration: waiting %lu ms", remainingTime);
            LogBox::end();
            delay(remainingTime);
        }
    }
    
    LogBox::line("Turning off frontlight");
    
    // Set brightness to 0 before disabling circuit
    _display->setFrontlight(0);
    
    // Disable frontlight circuit
    _display->frontlight(false);
    
    // Reset state
    _activated = false;
    _startTime = 0;
    _currentBrightness = 0;
    _minDurationMs = 0;
}

bool FrontlightManager::isActive() const {
    return _activated;
}

unsigned long FrontlightManager::getElapsedTime() const {
    if (!_activated) {
        return 0;
    }
    return millis() - _startTime;
}

void FrontlightManager::reset() {
    _activated = false;
    _startTime = 0;
    _currentBrightness = 0;
    _minDurationMs = 0;
}

#endif // HAS_FRONTLIGHT
