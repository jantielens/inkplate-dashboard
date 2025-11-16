#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

/**
 * Logger - Indentation-based logger for visually grouped serial output
 * 
 * Supports nested operations (up to 3 levels) with automatic indentation
 * and timing for each block.
 * 
 * Example usage:
 *   Logger::begin("WiFi");
 *   Logger::line("SSID: MyNetwork");
 *   Logger::begin("Channel Lock");
 *   Logger::line("Using channel 6");
 *   Logger::end();  // Shows "Done (Xms)"
 *   Logger::line("Connected!");
 *   Logger::end();  // Shows "Done (Yms)"
 * 
 * Example output:
 *   [WiFi] Starting...
 *     SSID: MyNetwork
 *     [Channel Lock] Starting...
 *       Using channel 6
 *     Done (45ms)
 *     Connected!
 *   Done (1234ms)
 */
class Logger {
public:
    // Begin a log block with a module name
    static void begin(const char* module);
    static void begin(const String& module);
    
    // Add a content line to the current block
    static void line(const char* message);
    static void line(const String& message);
    
    // Add a formatted content line (printf-style)
    static void linef(const char* format, ...) __attribute__((format(printf, 1, 2)));
    
    // End the current log block
    // Call with no argument for "Done", or provide a custom message
    static void end(const char* message = nullptr);
    static void end(const String& message);
    
    // Convenience method: combines begin + line + end for single-line messages
    static void message(const char* module, const char* msg);
    static void message(const String& module, const String& msg);
    static void messagef(const char* module, const char* format, ...) __attribute__((format(printf, 2, 3)));

#ifdef UNIT_TEST
    // Testing support
    static void resetForTesting();
#endif

private:
    static unsigned long startTimes[3];  // Stack for up to 3 nesting levels
    static uint8_t nestLevel;            // Current nesting level (0-3)
    static const char* indent();         // Returns indentation string based on nestLevel
};

#endif // LOGGER_H
