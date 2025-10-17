#include "logger.h"
#include <stdarg.h>

void LogBox::begin(const char* title) {
    Serial.print("+-- ");
    Serial.println(title);
}

void LogBox::begin(const String& title) {
    begin(title.c_str());
}

void LogBox::line(const char* message) {
    Serial.print("|   ");
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
    
    Serial.print("|   ");
    Serial.println(buffer);
}

void LogBox::end(const char* message) {
    Serial.print("+-- ");
    Serial.println(message);
}

void LogBox::end(const String& message) {
    end(message.c_str());
}
