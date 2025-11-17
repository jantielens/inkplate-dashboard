#ifndef SCREEN_H
#define SCREEN_H

#include "ui_base.h"
#include <Arduino.h>

/**
 * Screen - Declarative screen builder for e-ink displays
 * 
 * Eliminates manual Y-position tracking and code duplication across UI screens.
 * Provides fluent API for building screens with automatic layout.
 * Logo and battery are enabled by default (when overlayManager is provided).
 * 
 * Example usage:
 *   // Logo + battery (default)
 *   Screen(displayManager, overlayManager, batteryVoltage)
 *     .addHeading1("Configuration Mode")
 *     .addText("Visit the configuration portal")
 *     .display();
 *   
 *   // Logo only (no battery)
 *   Screen(displayManager)
 *     .addHeading1("OTA Update")
 *     .display();
 *   
 *   // Explicit opt-out
 *   Screen(displayManager, overlayManager, batteryVoltage)
 *     .withoutLogo()
 *     .addText("Minimal screen")
 *     .display();
 */
class Screen : public UIBase {
public:
    Screen(DisplayManager* displayManager, OverlayManager* overlayMgr = nullptr, float batteryVoltage = 0.0f);
    ~Screen();
    
    // Configuration methods (return *this for chaining)
    Screen& withoutLogo();      // Disable logo (logo enabled by default)
    Screen& withoutBattery();   // Disable battery (battery enabled by default if overlayManager provided)
    Screen& withRotation();  // Enable rotation (default: enabled)
    Screen& withoutRotation();  // Disable rotation
    
    // Content methods (return *this for chaining)
    Screen& addHeading1(const String& text);  // Large heading
    Screen& addHeading2(const String& text);  // Medium heading
    Screen& addText(const String& text);  // Body text
    Screen& addSpacing(int pixels = LINE_SPACING);  // Add vertical space
    Screen& addKeyValue(const String& key, const String& value);  // "Key: Value"
    Screen& addNumberedItem(uint8_t number, const String& text);  // "1. Text"
    
    // Render method (call last)
    void display();
    
private:
    bool _showLogo;
    bool _showBattery;
    bool _enableRotation;
    int _currentY;
    float _batteryVoltage;
    
    void drawLogo();
    void drawBattery();
    void ensureLineHeight(const GFXfont* font);
};

#endif // SCREEN_H
