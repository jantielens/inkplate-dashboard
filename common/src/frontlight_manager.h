#ifndef FRONTLIGHT_MANAGER_H
#define FRONTLIGHT_MANAGER_H

#include <Arduino.h>
#include "Inkplate.h"
#include "board_config.h"

#if defined(HAS_FRONTLIGHT) && HAS_FRONTLIGHT == true

/**
 * @brief Manages frontlight state and operations for boards with frontlight support
 * 
 * Encapsulates all frontlight-related state and provides clean interface for:
 * - Turning frontlight on/off
 * - Tracking activation time and duration
 * - Ensuring minimum display duration before sleep
 */
class FrontlightManager {
private:
    Inkplate* _display;
    bool _activated;
    unsigned long _startTime;
    uint8_t _currentBrightness;
    unsigned long _minDurationMs;
    
public:
    /**
     * @brief Construct a new Frontlight Manager
     * @param display Pointer to Inkplate display object
     */
    FrontlightManager(Inkplate* display);
    
    /**
     * @brief Turn on the frontlight
     * @param brightness Brightness level (0-63, where 63 is maximum)
     * @param minDurationMs Minimum time to keep frontlight on (0 = no minimum)
     */
    void turnOn(uint8_t brightness, unsigned long minDurationMs = 0);
    
    /**
     * @brief Turn off the frontlight, respecting minimum duration if set
     * If minimum duration hasn't elapsed, this method will block until it has
     */
    void turnOff();
    
    /**
     * @brief Check if frontlight is currently active
     * @return true if frontlight is on
     */
    bool isActive() const;
    
    /**
     * @brief Get elapsed time since frontlight was turned on
     * @return Elapsed time in milliseconds (0 if not active)
     */
    unsigned long getElapsedTime() const;
    
    /**
     * @brief Reset state (used when waking from deep sleep)
     */
    void reset();
};

#endif // HAS_FRONTLIGHT

#endif // FRONTLIGHT_MANAGER_H
