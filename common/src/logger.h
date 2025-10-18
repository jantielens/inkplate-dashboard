#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

/**
 * LogBox - Unicode box-drawing logger for visually grouped serial output
 * 
 * Uses Unicode rounded box characters for elegant terminal display:
 * ╭── Title
 * │   Line content
 * ╰─────────────────────────────────────────────
 * 
 * Example usage:
 *   LogBox::begin("Connecting to WiFi");
 *   LogBox::line("SSID: MyNetwork");
 *   LogBox::linef("Attempt: %d/%d", 1, 3);
 *   LogBox::end();  // Ends with horizontal line
 */
class LogBox {
public:
    // Begin a log box with a title
    static void begin(const char* title);
    static void begin(const String& title);
    
    // Add a content line to the current box
    static void line(const char* message);
    static void line(const String& message);
    
    // Add a formatted content line (printf-style)
    static void linef(const char* format, ...) __attribute__((format(printf, 1, 2)));
    
    // End the current log box
    // Call with no argument for horizontal line, or provide a custom message
    static void end(const char* message = "");
    static void end(const String& message);

private:
    static unsigned long startTime;  // Tracks time when begin() was called
};

#endif // LOGGER_H
