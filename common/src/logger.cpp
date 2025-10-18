#include "logger.h"
#include <stdarg.h>

// Initialize static member
unsigned long LogBox::startTime = 0;

void LogBox::begin(const char* title) {
    startTime = millis();
    Serial.print("╭── ");
    Serial.println(title);
}

void LogBox::begin(const String& title) {
    begin(title.c_str());
}

void LogBox::line(const char* message) {
    Serial.print("│   ");
    Serial.println(message);
}

void LogBox::line(const String& message) {
    line(message.c_str());
}

void LogBox::linef(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Serial.print("│   ");
    Serial.println(buffer);
}

void LogBox::end(const char* message) {
    unsigned long elapsed = millis() - startTime;
    
    if (message == nullptr || strlen(message) == 0) {
        // Format: ╰──────(NNNNms)───────
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "╰──────(%lums)───────", elapsed);
        Serial.println(buffer);
    } else {
        Serial.print("╰── ");
        Serial.print(message);
        Serial.print(" (");
        Serial.print(elapsed);
        Serial.println("ms)");
    }
}

void LogBox::end(const String& message) {
    if (message.length() > 0) {
        end(message.c_str());
    } else {
        end("");
    }
}
