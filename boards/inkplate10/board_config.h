#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// Board identification
#define BOARD_NAME "Inkplate 10"
#define BOARD_TYPE INKPLATE_10

// Display specifications
#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 820
#define BOARD_ROTATION 0  // 0, 1, 2, or 3

// Display mode
#define DISPLAY_MODE INKPLATE_3BIT  // 3-bit grayscale

// Display performance optimization flags
#define DISPLAY_FAST_REFRESH true    // Inkplate 10 has reasonably fast refresh
#define DISPLAY_MINIMAL_UI false     // Show full UI with logos and intermediate screens

// Board-specific features
#define HAS_TOUCHSCREEN true
#define HAS_FRONTLIGHT false
#define HAS_BATTERY true
#define HAS_BUTTON true  // Inkplate 10 has a wake button

// Board-specific pins
// Inkplate 10 specific pins
#define WAKE_BUTTON_PIN 36  // GPIO36 - Wake button for config mode (assuming same as 5V2)

// Battery monitoring pins
#define BATTERY_ADC_PIN 35      // GPIO35 - ADC pin for battery voltage reading
// Note: Inkplate boards don't use a MOSFET - battery is always connected to ADC

// Board-specific settings
#define DISPLAY_TIMEOUT_MS 15000  // Larger display, longer timeout

// Font sizes for text hierarchy
#if !DISPLAY_MINIMAL_UI
  // Custom fonts for larger displays - use special marker values
  // These will be mapped to actual GFXfont pointers in display_manager.cpp
  #define FONT_HEADING1 0xF001  // FreeSansBold24pt7b
  #define FONT_HEADING2 0xF002  // FreeSans18pt7b
  #define FONT_NORMAL 0xF003    // FreeSans12pt7b (increased from 9pt for better readability)
#else
  // Keep simple integer sizes for Inkplate 2
  #define FONT_HEADING1 8   // Large headings (e.g., "Dashboard", screen titles)
  #define FONT_HEADING2 5   // Medium headings (e.g., section titles)
  #define FONT_NORMAL 3     // Normal text (e.g., descriptions, status messages)
#endif

// Line spacing (pixels between lines of text)
#define LINE_SPACING 10

// Margins
#define MARGIN 10          // General margin (left, top)
#define INDENT_MARGIN 30   // Indentation for nested content

#endif // BOARD_CONFIG_H
