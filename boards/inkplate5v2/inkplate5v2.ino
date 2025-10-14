/*
   Inkplate5V2 Dashboard
   
   Multi-board dashboard project for Inkplate devices.
   This is the Inkplate 5 V2 specific sketch.
   
   Common implementation is in ../../common/main_sketch.ino.inc
   Board-specific configuration is in board_config.h.
*/

// Board validation
#ifndef ARDUINO_INKPLATE5V2
#error "Wrong board selection for this sketch, please select Soldered Inkplate5V2 in the boards menu."
#endif

// Include board configuration first (required by shared code)
#include "board_config.h"

// Include shared implementation
#include <src/main_sketch.ino.inc>
