#include "utils.h"

namespace Utils {

String formatTime(unsigned long milliseconds) {
    unsigned long seconds = milliseconds / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    seconds = seconds % 60;
    minutes = minutes % 60;
    
    char buffer[32];
    sprintf(buffer, "%02lu:%02lu:%02lu", hours, minutes, seconds);
    return String(buffer);
}

String formatMemory(size_t bytes) {
    if (bytes < 1024) {
        return String(bytes) + " B";
    } else if (bytes < 1024 * 1024) {
        return String(bytes / 1024.0, 1) + " KB";
    } else {
        return String(bytes / (1024.0 * 1024.0), 1) + " MB";
    }
}

void printDebug(const char* message) {
    Serial.println(message);
}

void printDebug(const String& message) {
    Serial.println(message);
}

String getBoardName() {
    // Board name should be passed from sketch or defined externally
    return "Inkplate";
}

String getChipInfo() {
    String info = "ESP32";
    #ifdef ESP_ARDUINO_VERSION
    info += " Core: " + String(ESP_ARDUINO_VERSION_MAJOR) + "." + String(ESP_ARDUINO_VERSION_MINOR);
    #endif
    return info;
}

}
