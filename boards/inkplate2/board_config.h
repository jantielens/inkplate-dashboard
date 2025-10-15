#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// Board identification
#define BOARD_NAME "Inkplate 2"
#define BOARD_TYPE INKPLATE_2

// Display specifications
#define SCREEN_WIDTH 212
#define SCREEN_HEIGHT 104
#define BOARD_ROTATION 0  // 0 = landscape orientation

// Display mode
// Note: Inkplate 2 does not use INKPLATE_1BIT/INKPLATE_3BIT in constructor
// The display mode is set automatically by the hardware
#define DISPLAY_MODE_INKPLATE2  // Special flag for Inkplate 2

// Board-specific features
#define HAS_TOUCHSCREEN false
#define HAS_FRONTLIGHT false
#define HAS_BATTERY true
#define HAS_BUTTON false  // Inkplate 2 does not have a physical button

// Board-specific pins
// Note: Inkplate 2 does not have a physical button, but we define a dummy pin
// to maintain code compatibility. Button wake functionality will not work.
#define WAKE_BUTTON_PIN 36  // Dummy pin (not physically connected to a button)

// Battery monitoring pins
#define BATTERY_ADC_PIN 35      // GPIO35 - ADC pin for battery voltage reading
// Note: Inkplate boards don't use a MOSFET - battery is always connected to ADC

// Board-specific settings
#define DISPLAY_TIMEOUT_MS 8000  // Smaller display, shorter timeout

// Font and layout scaling for small screen
// Inkplate 2 is ~1/6 the height of larger boards (104 vs 720-825)
#define FONT_SCALE_FACTOR 0.33    // Scale fonts to 33% of original
#define LAYOUT_SCALE_FACTOR 0.20  // Scale Y positions to 20% (104/720 ≈ 0.14, round up for readability)

// Margins (no margins on tiny screen to maximize space)
#define LEFT_MARGIN 0
#define INDENT_MARGIN 0

#endif // BOARD_CONFIG_H
