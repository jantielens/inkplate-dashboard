#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>

// Utility functions shared across all boards

namespace Utils {
    // String formatting helpers
    String formatTime(unsigned long milliseconds);
    String formatMemory(size_t bytes);
    
    // Debugging helpers
    void printDebug(const char* message);
    void printDebug(const String& message);
    
    // Board info
    String getBoardName();
    String getChipInfo();
}

#endif // UTILS_H
