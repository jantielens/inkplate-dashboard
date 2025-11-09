#ifndef FONTS_H
#define FONTS_H

// This header should be included in .cpp files that use custom fonts
// It must be included AFTER Inkplate.h to ensure GFXfont types are available

#include "board_config.h"

#if !DISPLAY_MINIMAL_UI
  // Include custom font files for larger displays
  #include <fonts/FreeSans9pt7b.h>
  #include <fonts/FreeSans12pt7b.h>
  #include <fonts/FreeSans18pt7b.h>
  #include <fonts/FreeSansBold18pt7b.h>
  #include <fonts/FreeSansBold24pt7b.h>
#endif

#endif // FONTS_H
