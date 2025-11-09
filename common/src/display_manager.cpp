// ...existing code...
#include "board_config.h"
#include "display_manager.h"
#include "logger.h"
#include <src/version.h>
#include <Wire.h>

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

#ifndef DISPLAY_MODE_INKPLATE2
// VCOM management functions (not available on Inkplate 2 - no TPS65186 PMIC)

// DANGEROUS: Programs the VCOM value in the TPS65186 EEPROM. Use only if you know what you are doing.
// vcom: negative voltage in volts (e.g., -1.23). Returns true if successful.
bool DisplayManager::programPanelVCOM(double vcom, String* diagnostics) {
    // Limit range for safety
    if (vcom > 0.0 || vcom < -5.0) {
        Serial.println("VCOM out of range: " + String(vcom));
        return false;
    }
    int raw = (int)(-vcom * 100.0 + 0.5); // Convert to 0..500
    if (raw < 0 || raw > 511) {
        Serial.println("VCOM raw value out of range: " + String(raw));
        return false;
    }
    uint8_t lsb = raw & 0xFF;
    uint8_t msb = (raw >> 8) & 0x01;

    Wire.begin();
    _display->einkOn();
    delay(10);

    LogBox::begin("VCOM Programming");
    
    String localDiag;
    auto log = [&](const String& msg) {
        LogBox::line(msg);
        localDiag += msg + "<br>";
    };

    log("Programming requested: " + String(vcom, 3) + " V (raw=" + String(raw) + ")");

    // Read current VCOM registers before programming
    Wire.beginTransmission(0x48);
    Wire.write((uint8_t)0x03);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)0x48, (uint8_t)1);
    uint8_t preVcomL = Wire.available() ? Wire.read() : 0xFF;
    Wire.beginTransmission(0x48);
    Wire.write((uint8_t)0x04);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)0x48, (uint8_t)1);
    uint8_t preVcomH = Wire.available() ? Wire.read() : 0xFF;
    log("Pre-program: 0x03=" + String(preVcomL) + ", 0x04=" + String(preVcomH));

    // Write LSB to 0x03
    Wire.beginTransmission(0x48);
    Wire.write((uint8_t)0x03);
    Wire.write(lsb);
    uint8_t result = Wire.endTransmission();
    log("Write 0x03: lsb=" + String(lsb) + ", result=" + String(result));
    // Immediate readback to confirm LSB write
    Wire.beginTransmission(0x48);
    Wire.write((uint8_t)0x03);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)0x48, (uint8_t)1);
    uint8_t rbL = Wire.available() ? Wire.read() : 0xFF;
    log("Readback 0x03 after write: " + String(rbL));

    // Write MSB to 0x04 (preserve other bits)
    Wire.beginTransmission(0x48);
    Wire.write((uint8_t)0x04);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)0x48, (uint8_t)1);
    uint8_t r4 = Wire.available() ? Wire.read() : 0xFF;
    log("Read 0x04 before MSB write: " + String(r4));
    r4 = (r4 & ~0x01) | msb;
    Wire.beginTransmission(0x48);
    Wire.write((uint8_t)0x04);
    Wire.write(r4);
    result = Wire.endTransmission();
    log("Write 0x04: msb=" + String(msb) + ", result=" + String(result));
    // Immediate readback to confirm MSB write
    Wire.beginTransmission(0x48);
    Wire.write((uint8_t)0x04);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)0x48, (uint8_t)1);
    uint8_t rbR4 = Wire.available() ? Wire.read() : 0xFF;
    log("Readback 0x04 after MSB write: " + String(rbR4));

    // Set bit 6 to program EEPROM
    Wire.beginTransmission(0x48);
    Wire.write((uint8_t)0x04);
    Wire.write(r4 | (1 << 6));
    result = Wire.endTransmission();
    log("Write 0x04 (EEPROM program bit): result=" + String(result));
    
    // Wait for EEPROM programming to complete by polling bit 6
    // The program bit should auto-clear when programming is done
    unsigned long start = millis();
    bool programmed = false;
    while (millis() - start < 1000) {
        delay(10);
        Wire.beginTransmission(0x48);
        Wire.write((uint8_t)0x04);
        Wire.endTransmission(false);
        Wire.requestFrom((uint8_t)0x48, (uint8_t)1);
        uint8_t r4Status = Wire.available() ? Wire.read() : 0xFF;
        if ((r4Status & (1 << 6)) == 0) {
            // Program bit cleared - programming complete
            programmed = true;
            log("EEPROM programming complete (bit 6 cleared after " + String(millis() - start) + "ms)");
            break;
        }
    }
    
    if (!programmed) {
        log("Warning: Program bit did not clear within timeout");
    }

    // After programming, zero the volatile registers to prepare for EEPROM reload
    // The TPS65186 will reload VCOM from EEPROM when we cycle power
    Wire.beginTransmission(0x48);
    Wire.write((uint8_t)0x03);
    Wire.write(0);
    result = Wire.endTransmission();
    log("Clear volatile 0x03: result=" + String(result));
    Wire.beginTransmission(0x48);
    Wire.write((uint8_t)0x04);
    Wire.write(0);
    result = Wire.endTransmission();
    log("Clear volatile 0x04: result=" + String(result));

    // Power cycle to force TPS65186 to reload VCOM from EEPROM
    // Need sufficient time for chip to fully power down and reinitialize
    _display->einkOff();
    delay(100);  // Longer delay for complete power down
    _display->einkOn();
    delay(100);  // Longer delay for chip to reinitialize and reload from EEPROM

    // Verify: read back the LSB/MSB from TPS65186 (now loaded from EEPROM)
    Wire.beginTransmission(0x48);
    Wire.write((uint8_t)0x03);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)0x48, (uint8_t)1);
    uint8_t vL = Wire.available() ? Wire.read() : 0xFF;
    Wire.beginTransmission(0x48);
    Wire.write((uint8_t)0x04);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)0x48, (uint8_t)1);
    uint8_t vH = Wire.available() ? (Wire.read() & 0x01) : 0xFF;
    int check = (vH << 8) | vL;
    log("Post-reload from EEPROM: 0x03=" + String(vL) + ", 0x04=" + String(vH) + ", check=" + String(check));
    if (check != raw) {
        log("Verify failed: wrote " + String(raw) + ", read back " + String(check));
        LogBox::end();
        if (diagnostics) *diagnostics = localDiag;
        return false;
    }
    log("Programming successful: " + String(vcom, 3) + " V");
    LogBox::end();
    if (diagnostics) *diagnostics = localDiag;
    return true;
}

// Read the panel's VCOM value from the TPS65186 via I2C (address 0x48)
// The value is stored as a 9-bit unsigned integer (0..511) representing
// the absolute VCOM in 1/100 V steps. The register pair is 0x03 (LSB) and
// 0x04 bit0 (MSB). The returned VCOM is negative (e.g., -1.23 V).
double DisplayManager::readPanelVCOM() {
    // Ensure I2C is initialized
    Wire.begin();

    // Wake the TPS65186 so registers will respond
    _display->einkOn();
    delay(10);

    // Read LSB (register 0x03)
    Wire.beginTransmission(0x48);
    Wire.write((uint8_t)0x03);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)0x48, (uint8_t)1);
    if (Wire.available() == 0) {
        // Power down and return NaN on error
        _display->einkOff();
        return NAN;
    }
    uint8_t vcomL = Wire.read();

    // Read MSB (register 0x04) and mask bit 0
    Wire.beginTransmission(0x48);
    Wire.write((uint8_t)0x04);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)0x48, (uint8_t)1);
    if (Wire.available() == 0) {
        _display->einkOff();
        return NAN;
    }
    uint8_t vcomH = Wire.read() & 0x01;

    // Power down the TPS65186
    _display->einkOff();
    delay(10);

    int raw = (vcomH << 8) | vcomL; // 0 - 511
    // Value is stored as hundredths of a volt
    double v = -(raw / 100.0);
    return v;
}

// Display VCOM test pattern with grayscale bars (3-bit mode on supported devices)
void DisplayManager::showVcomTestPattern() {
    LogBox::begin("VCOM Test Pattern");
    
    // Switch to 3-bit mode for grayscale display (not available on Inkplate 2)
    #ifndef DISPLAY_MODE_INKPLATE2
    _display->selectDisplayMode(INKPLATE_3BIT);
    #endif
    _display->clearDisplay();
    
    // Read and display current VCOM
    double currentVcom = readPanelVCOM();
    _display->setTextColor(BLACK);
    _display->setTextSize(2);
    _display->setCursor(5, 5);
    _display->print("Current VCOM: ");
    if (!isnan(currentVcom)) {
        _display->print(currentVcom, 2);
        _display->print(" V");
        LogBox::linef("Current VCOM: %.2f V", currentVcom);
    } else {
        _display->print("N/A");
        LogBox::line("Current VCOM: N/A (read failed)");
    }
    
    #ifdef DISPLAY_MODE_INKPLATE2
    // Inkplate 2 only supports 1-bit mode (black/white)
    // Draw simple checkerboard pattern to test contrast
    int barWidth = getWidth() / 4;
    for (int i = 0; i < 4; i++) {
        int x = barWidth * i;
        if (i % 2 == 0) {
            _display->fillRect(x, 40, barWidth, getHeight() - 40, BLACK);
        }
    }
    LogBox::line("Displaying test pattern (1-bit checkerboard for Inkplate 2)");
    #else
    // Draw 8 vertical grayscale bars (0-7 in 3-bit = 8 gray levels)
    int barWidth = getWidth() / 8;
    for (int i = 0; i < 8; i++) {
        int x = barWidth * i;
        _display->fillRect(x, 40, barWidth, getHeight() - 40, i);
    }
    LogBox::line("Displaying test pattern with 8 grayscale bars");
    #endif
    
    _display->display();
    LogBox::end();
}

#endif // DISPLAY_MODE_INKPLATE2

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
#if !DISPLAY_MINIMAL_UI
    // For larger displays, textSize is actually a GFXfont pointer
    if (textSize != 0) {
        _display->setFont((const GFXfont*)textSize);
        _display->setTextSize(1);  // Always use size 1 with custom fonts
        
        // GFXfonts position text using baseline, not top-left
        // Adjust y to account for this by adding the font's ascent
        const GFXfont* font = (const GFXfont*)textSize;
        // yAdvance is the total line height, glyph->yOffset is typically negative (above baseline)
        // We need to shift down by the absolute value of the maximum ascent
        // For simplicity, use yAdvance as it represents the full line height
        y += font->yAdvance;
    }
#else
    // For Inkplate 2, textSize is an integer scale factor
    _display->setFont();  // Reset to default font
    _display->setTextSize(textSize);
#endif
    _display->setTextColor(BLACK);
    _display->setCursor(x, y);
    _display->print(message);
}

void DisplayManager::drawCentered(const char* message, int y, int textSize) {
#if !DISPLAY_MINIMAL_UI
    // For larger displays, textSize is actually a GFXfont pointer
    if (textSize != 0) {
        _display->setFont((const GFXfont*)textSize);
        _display->setTextSize(1);  // Always use size 1 with custom fonts
    }
#else
    // For Inkplate 2, textSize is an integer scale factor
    _display->setFont();  // Reset to default font
    _display->setTextSize(textSize);
#endif
    _display->setTextColor(BLACK);
    
    // Calculate text width (approximate)
    int16_t x1, y1;
    uint16_t w, h;
    _display->getTextBounds(message, 0, 0, &x1, &y1, &w, &h);
    
    int x = (getWidth() - w) / 2;
    
#if !DISPLAY_MINIMAL_UI
    // Adjust y for baseline positioning with custom fonts
    if (textSize != 0) {
        const GFXfont* font = (const GFXfont*)textSize;
        y += font->yAdvance;
    }
#endif
    
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
    
#if !DISPLAY_MINIMAL_UI
    // For larger displays, FONT_NORMAL is a GFXfont pointer
    _display->setFont((const GFXfont*)FONT_NORMAL);
    _display->setTextSize(1);  // Always use size 1 with custom fonts
#else
    // For Inkplate 2, FONT_NORMAL is an integer scale factor
    _display->setFont();  // Reset to default font
    _display->setTextSize(FONT_NORMAL);
#endif
    _display->setTextColor(BLACK);

    int16_t x1 = 0;
    int16_t y1 = 0;
    uint16_t w = 0;
    uint16_t h = 0;
    _display->getTextBounds(versionLabel, 0, 0, &x1, &y1, &w, &h);

    int x = getWidth() - w - MARGIN;
    int y = getHeight() - MARGIN;
    
#if !DISPLAY_MINIMAL_UI
    // GFXfonts use baseline positioning - y1 from getTextBounds is the offset above baseline
    // We want the bottom of the text to be at (getHeight() - MARGIN)
    // So we need to subtract the descent (which is typically positive in the bounds)
    y -= h;
#else
    // Default font uses top-left positioning
    y -= h;
#endif
    
    if (x < MARGIN) {
        x = MARGIN;
    }
    if (y < MARGIN) {
        y = MARGIN;
    }

    _display->setCursor(x, y);
    _display->print(versionLabel);
}

void DisplayManager::drawBitmap(const uint8_t* bitmap, int x, int y, int w, int h) {
    if (!bitmap || w <= 0 || h <= 0) return;

    // Use Inkplate's drawImage for embedded bitmaps
    _display->drawImage(bitmap, x, y, w, h);
}

// Helper to calculate approximate font height in pixels
int DisplayManager::getFontHeight(int textSize) {
#if !DISPLAY_MINIMAL_UI
    // For larger displays, textSize is actually a GFXfont pointer
    if (textSize != 0) {
        const GFXfont* font = (const GFXfont*)textSize;
        // GFXfont height = yAdvance (line height)
        return font->yAdvance;
    }
    return 8;  // Fallback to default font height
#else
    // For Inkplate 2, use simple calculation for pixel fonts
    return 8 * textSize;  // 7 pixels + 1 pixel spacing, multiplied by size
#endif
}
