#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// Board identification
#define BOARD_NAME "Inkplate 6 Flick"
#define BOARD_TYPE INKPLATE_6FLICK

// Display specifications
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 758
#define BOARD_ROTATION 0  // 0, 1, 2, or 3

// Display mode
#define DISPLAY_MODE INKPLATE_3BIT

// Board-specific features
#define HAS_TOUCHSCREEN false
#define HAS_FRONTLIGHT false
#define HAS_BATTERY true
#define HAS_BUTTON true  // Inkplate 6 Flick has a wake button

// Board-specific pins
#define WAKE_BUTTON_PIN 36  // GPIO36 - Wake button for config mode

// Battery monitoring pins
#define BATTERY_ADC_PIN 35      // GPIO35 - ADC pin for battery voltage reading
// Note: Inkplate boards don't use a MOSFET - battery is always connected to ADC

// Board-specific settings
#define DISPLAY_TIMEOUT_MS 10000

// Font sizes for text hierarchy
#define FONT_HEADING1 6   // Large headings (e.g., "Dashboard", screen titles)
#define FONT_HEADING2 4   // Medium headings (e.g., section titles)
#define FONT_NORMAL 2     // Normal text (e.g., descriptions, status messages)

// Line spacing (pixels between lines of text)
#define LINE_SPACING 10

// Margins
#define MARGIN 20          // General margin (left, top)
#define INDENT_MARGIN 30   // Indentation for nested content

#endif // BOARD_CONFIG_H
