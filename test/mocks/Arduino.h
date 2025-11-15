// Mock Arduino/ESP32 types for unit testing
// This allows testing pure C++ decision functions without Arduino dependencies

#ifndef MOCK_ARDUINO_H
#define MOCK_ARDUINO_H

#include <stdint.h>
#include <string>
#include <ctime>

// Mock Arduino String class
class String {
public:
    std::string _data;
    
    String() : _data("") {}
    String(const char* str) : _data(str ? str : "") {}
    String(const std::string& str) : _data(str) {}
    String(int value) : _data(std::to_string(value)) {}
    
    const char* c_str() const { return _data.c_str(); }
    size_t length() const { return _data.length(); }
    
    bool operator==(const String& other) const { return _data == other._data; }
    
    // Concatenation operators
    String operator+(const String& other) const {
        String result;
        result._data = _data + other._data;
        return result;
    }
    
    String operator+(const char* s) const {
        String result;
        result._data = _data + s;
        return result;
    }
};

// Mock millis() - not used in decision functions but needed for compilation
inline unsigned long millis() { return 0; }

// Mock delay() - not used in decision functions
inline void delay(unsigned long ms) {}

#endif // MOCK_ARDUINO_H
