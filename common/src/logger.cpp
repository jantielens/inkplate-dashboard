#include "logger.h"
#include <stdarg.h>

// Initialize static members
unsigned long Logger::startTimes[3] = {0, 0, 0};
uint8_t Logger::nestLevel = 0;

const char* Logger::indent() {
    // Return indentation string based on current nesting level
    // Each level adds 2 spaces
    static const char* indents[] = {
        "",         // Level 0: no indent
        "  ",       // Level 1: 2 spaces
        "    ",     // Level 2: 4 spaces
        "      "    // Level 3+: 6 spaces
    };
    
    uint8_t level = nestLevel;
    if (level > 3) level = 3; // Cap at 3 for indentation
    return indents[level];
}

void Logger::begin(const char* module) {
    Serial.print(indent());
    Serial.print("[");
    Serial.print(module);
    Serial.println("] Starting...");
    
    // Save start time if we haven't exceeded max depth
    if (nestLevel < 3) {
        startTimes[nestLevel] = millis();
    }
    
    // Increment nesting level (but don't overflow)
    if (nestLevel < 255) { // Prevent uint8_t overflow
        nestLevel++;
    }
}

void Logger::begin(const String& module) {
    begin(module.c_str());
}

void Logger::line(const char* message) {
    Serial.print(indent());
    Serial.println(message);
}

void Logger::line(const String& message) {
    line(message.c_str());
}

void Logger::linef(const char* format, ...) {
    char buffer[128]; // Reduced from 256 bytes
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Serial.print(indent());
    Serial.println(buffer);
}

void Logger::end(const char* message) {
    // Decrement nesting level first (but don't underflow)
    if (nestLevel > 0) {
        nestLevel--;
    } else {
        // Extra end() calls are ignored gracefully
        return;
    }
    
    // Calculate elapsed time (0ms if we exceeded max depth)
    unsigned long elapsed = 0;
    if (nestLevel < 3) {
        elapsed = millis() - startTimes[nestLevel];
    }
    
    // Print end message with timing
    const char* msg = (message && strlen(message) > 0) ? message : "Done";
    Serial.print(indent());
    Serial.print(msg);
    Serial.print(" (");
    Serial.print(elapsed);
    Serial.println("ms)");
}

void Logger::end(const String& message) {
    if (message.length() > 0) {
        end(message.c_str());
    } else {
        end(nullptr);
    }
}

// Convenience methods for single-line messages
void Logger::message(const char* module, const char* msg) {
    begin(module);
    line(msg);
    end();
}

void Logger::message(const String& module, const String& msg) {
    message(module.c_str(), msg.c_str());
}

void Logger::messagef(const char* module, const char* format, ...) {
    begin(module);
    
    char buffer[128]; // Reduced from 256 bytes
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    line(buffer);
    end();
}

#ifdef UNIT_TEST
void Logger::resetForTesting() {
    nestLevel = 0;
    for (int i = 0; i < 3; i++) {
        startTimes[i] = 0;
    }
}
#endif
