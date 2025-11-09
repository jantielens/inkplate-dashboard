#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// Board identification
#define BOARD_NAME "Inkplate 5 V2"
#define BOARD_TYPE INKPLATE_5V2

// Display specifications
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define BOARD_ROTATION 0  // 0, 1, 2, or 3

// Display mode
#define DISPLAY_MODE INKPLATE_3BIT

// Display performance optimization flags
#define DISPLAY_FAST_REFRESH true    // Inkplate 5 V2 has reasonably fast refresh
#define DISPLAY_MINIMAL_UI false     // Show full UI with logos and intermediate screens

// Board-specific features
#define HAS_TOUCHSCREEN false
#define HAS_FRONTLIGHT false
#define HAS_BATTERY true
#define HAS_BUTTON true  // Inkplate 5 V2 has a wake button

// Board-specific pins
#define WAKE_BUTTON_PIN 36  // GPIO36 - Wake button for config mode

// Battery monitoring pins
#define BATTERY_ADC_PIN 35      // GPIO35 - ADC pin for battery voltage reading
// Note: Inkplate boards don't use a MOSFET - battery is always connected to ADC

// Board-specific settings
#define DISPLAY_TIMEOUT_MS 10000

// Font sizes for text hierarchy
#if !DISPLAY_MINIMAL_UI
  // Custom fonts for larger displays
  // Font pointers are cast to int to maintain API compatibility
  // The actual font definitions are in display_manager.cpp
  #define FONT_HEADING1 ((int)&FreeSansBold18pt7b)  // Large headings (e.g., "Dashboard", screen titles)
  #define FONT_HEADING2 ((int)&FreeSans12pt7b)      // Medium headings (e.g., section titles)
  #define FONT_NORMAL ((int)&FreeSans9pt7b)         // Normal text (e.g., descriptions, status messages)
#else
  // Keep simple integer sizes for Inkplate 2
  #define FONT_HEADING1 5   // Large headings (e.g., "Dashboard", screen titles)
  #define FONT_HEADING2 3   // Medium headings (e.g., section titles)
  #define FONT_NORMAL 2     // Normal text (e.g., descriptions, status messages)
#endif

// Line spacing (pixels between lines of text)
#define LINE_SPACING 10

// Margins
#define MARGIN 20          // General margin (left, top)
#define INDENT_MARGIN 30   // Indentation for nested content

#endif // BOARD_CONFIG_H

// Include custom fonts AFTER the header guard to ensure GFX types are available
// These fonts are only used on larger displays
#if !DISPLAY_MINIMAL_UI
#include <fonts/FreeSansBold18pt7b.h>
#include <fonts/FreeSans12pt7b.h>
#include <fonts/FreeSans9pt7b.h>
#endif
