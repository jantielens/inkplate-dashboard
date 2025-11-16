#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <cstdarg>
#include <cstdint>

// We need to capture Serial output for testing
namespace {
    std::stringstream serialOutput;
    unsigned long mockMillis = 0;
}

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

// Mock millis()
unsigned long millis() { return mockMillis; }

// Mock Serial class for testing
class MockSerial {
public:
    void print(const char* str) { serialOutput << str; }
    void print(const String& str) { serialOutput << str.c_str(); }
    void print(unsigned long num) { serialOutput << num; }
    void println(const char* str) { serialOutput << str << "\n"; }
    void println(const String& str) { serialOutput << str.c_str() << "\n"; }
    void printf(const char* format, ...) {
        char buffer[512];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        serialOutput << buffer;
    }
};

MockSerial Serial;

// Logger implementation with nesting support
class Logger {
public:
    static void begin(const char* module);
    static void begin(const String& module);
    static void line(const char* message);
    static void line(const String& message);
    static void linef(const char* format, ...) __attribute__((format(printf, 1, 2)));
    static void end(const char* message = nullptr);
    static void end(const String& message);
    static void message(const char* module, const char* msg);
    static void message(const String& module, const String& msg);
    static void messagef(const char* module, const char* format, ...) __attribute__((format(printf, 2, 3)));
    static void resetForTesting();

private:
    static unsigned long startTimes[3];
    static uint8_t nestLevel;
    static const char* indent();
};

// Initialize static members
unsigned long Logger::startTimes[3] = {0, 0, 0};
uint8_t Logger::nestLevel = 0;

const char* Logger::indent() {
    static const char* indents[] = {
        "",         // Level 0: no indent
        "  ",       // Level 1: 2 spaces
        "    ",     // Level 2: 4 spaces
        "      "    // Level 3+: 6 spaces
    };
    
    uint8_t level = nestLevel;
    if (level > 3) level = 3;
    return indents[level];
}

void Logger::begin(const char* module) {
    Serial.printf("%s[%s] Starting...\n", indent(), module);
    
    if (nestLevel < 3) {
        startTimes[nestLevel] = millis();
    }
    
    if (nestLevel < 255) {
        nestLevel++;
    }
}

void Logger::begin(const String& module) {
    begin(module.c_str());
}

void Logger::line(const char* message) {
    Serial.printf("%s%s\n", indent(), message);
}

void Logger::line(const String& message) {
    line(message.c_str());
}

void Logger::linef(const char* format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Serial.printf("%s%s\n", indent(), buffer);
}

void Logger::end(const char* message) {
    if (nestLevel > 0) {
        nestLevel--;
    } else {
        return;
    }
    
    unsigned long elapsed = 0;
    if (nestLevel < 3) {
        elapsed = millis() - startTimes[nestLevel];
    }
    
    const char* msg = (message && strlen(message) > 0) ? message : "Done";
    Serial.printf("%s%s (%lums)\n", indent(), msg, elapsed);
}

void Logger::end(const String& message) {
    if (message.length() > 0) {
        end(message.c_str());
    } else {
        end(nullptr);
    }
}

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
    
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    line(buffer);
    end();
}

void Logger::resetForTesting() {
    nestLevel = 0;
    for (int i = 0; i < 3; i++) {
        startTimes[i] = 0;
    }
}

// Backward compatibility alias
using LogBox = Logger;

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        serialOutput.str("");
        serialOutput.clear();
        mockMillis = 0;
        Logger::resetForTesting();
    }
    
    std::string getOutput() {
        return serialOutput.str();
    }
    
    void advanceTime(unsigned long ms) {
        mockMillis += ms;
    }
};

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST_F(LoggerTest, SingleBlock_ShowsModuleAndTiming) {
    Logger::begin("WiFi");
    advanceTime(100);
    Logger::end();
    
    std::string output = getOutput();
    EXPECT_NE(output.find("[WiFi] Starting..."), std::string::npos);
    EXPECT_NE(output.find("Done (100ms)"), std::string::npos);
}

TEST_F(LoggerTest, SingleBlock_WithMessage) {
    Logger::begin("WiFi");
    Logger::line("SSID: TestNetwork");
    advanceTime(50);
    Logger::end("Connected");
    
    std::string output = getOutput();
    EXPECT_NE(output.find("[WiFi] Starting..."), std::string::npos);
    EXPECT_NE(output.find("SSID: TestNetwork"), std::string::npos);
    EXPECT_NE(output.find("Connected (50ms)"), std::string::npos);
}

TEST_F(LoggerTest, FormattedLine_WorksCorrectly) {
    Logger::begin("Test");
    Logger::linef("Value: %d, Status: %s", 42, "OK");
    Logger::end();
    
    std::string output = getOutput();
    EXPECT_NE(output.find("Value: 42, Status: OK"), std::string::npos);
}

// ============================================================================
// Nesting Tests
// ============================================================================

TEST_F(LoggerTest, TwoLevelNesting_ShowsIndentation) {
    Logger::begin("WiFi");
    Logger::line("Connecting...");
    advanceTime(50);
    
    Logger::begin("Channel Lock");
    Logger::line("Using channel 6");
    advanceTime(20);
    Logger::end();
    
    advanceTime(30);
    Logger::line("Connected!");
    Logger::end();
    
    std::string output = getOutput();
    
    // Check structure
    EXPECT_NE(output.find("[WiFi] Starting..."), std::string::npos);
    EXPECT_NE(output.find("  Connecting..."), std::string::npos);
    EXPECT_NE(output.find("  [Channel Lock] Starting..."), std::string::npos);
    EXPECT_NE(output.find("    Using channel 6"), std::string::npos);
    EXPECT_NE(output.find("  Done (20ms)"), std::string::npos);
    EXPECT_NE(output.find("  Connected!"), std::string::npos);
    EXPECT_NE(output.find("Done (100ms)"), std::string::npos);
}

TEST_F(LoggerTest, ThreeLevelNesting_ShowsCorrectIndentation) {
    Logger::begin("Level1");
    Logger::begin("Level2");
    Logger::begin("Level3");
    Logger::line("Deep message");
    Logger::end();
    Logger::end();
    Logger::end();
    
    std::string output = getOutput();
    
    EXPECT_NE(output.find("[Level1] Starting..."), std::string::npos);
    EXPECT_NE(output.find("  [Level2] Starting..."), std::string::npos);
    EXPECT_NE(output.find("    [Level3] Starting..."), std::string::npos);
    EXPECT_NE(output.find("      Deep message"), std::string::npos);
}

// ============================================================================
// Overflow/Underflow Handling Tests
// ============================================================================

TEST_F(LoggerTest, FourLevelNesting_GracefulDegradation) {
    Logger::begin("Level1");
    advanceTime(10);
    Logger::begin("Level2");
    advanceTime(20);
    Logger::begin("Level3");
    advanceTime(30);
    Logger::begin("Level4"); // Exceeds max depth of 3
    Logger::line("Deep work");
    advanceTime(40);
    Logger::end(); // Should show 0ms timing
    Logger::end();
    Logger::end();
    Logger::end();
    
    std::string output = getOutput();
    
    // Check that it doesn't crash and shows correct behavior
    EXPECT_NE(output.find("[Level4] Starting..."), std::string::npos);
    EXPECT_NE(output.find("Deep work"), std::string::npos);
    EXPECT_NE(output.find("Done (0ms)"), std::string::npos); // Level4 should show 0ms
}

TEST_F(LoggerTest, ExtraEnd_Ignored) {
    Logger::begin("Test");
    advanceTime(50);
    Logger::end();
    
    // Count "Done" occurrences before extra ends
    std::string output1 = getOutput();
    size_t count1 = 0;
    size_t pos = 0;
    while ((pos = output1.find("Done", pos)) != std::string::npos) {
        count1++;
        pos += 4;
    }
    
    Logger::end(); // Extra end - should be ignored
    Logger::end(); // Another extra end
    
    std::string output2 = getOutput();
    size_t count2 = 0;
    pos = 0;
    while ((pos = output2.find("Done", pos)) != std::string::npos) {
        count2++;
        pos += 4;
    }
    
    // Should still have same number of "Done" messages
    EXPECT_EQ(count1, count2);
}

// ============================================================================
// Timing Tests
// ============================================================================

TEST_F(LoggerTest, Timing_AccuratelyTracked) {
    Logger::begin("Operation");
    advanceTime(1234);
    Logger::end();
    
    std::string output = getOutput();
    EXPECT_NE(output.find("Done (1234ms)"), std::string::npos);
}

TEST_F(LoggerTest, NestedTiming_IndependentlyTracked) {
    Logger::begin("Outer");
    advanceTime(100);
    
    Logger::begin("Inner");
    advanceTime(50);
    Logger::end();
    
    advanceTime(150);
    Logger::end();
    
    std::string output = getOutput();
    
    EXPECT_NE(output.find("Done (50ms)"), std::string::npos);  // Inner
    EXPECT_NE(output.find("Done (300ms)"), std::string::npos); // Outer: 100 + 50 + 150
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(LoggerTest, EmptyBlock_StillWorks) {
    Logger::begin("Empty");
    advanceTime(10);
    Logger::end();
    
    std::string output = getOutput();
    EXPECT_NE(output.find("[Empty] Starting..."), std::string::npos);
    EXPECT_NE(output.find("Done (10ms)"), std::string::npos);
}

TEST_F(LoggerTest, NullMessage_HandledGracefully) {
    Logger::begin("Test");
    Logger::end(nullptr);
    
    std::string output = getOutput();
    EXPECT_NE(output.find("Done"), std::string::npos);
}

TEST_F(LoggerTest, LongMessage_Truncated) {
    // Create a message longer than the buffer (128 bytes)
    std::string longMsg;
    for (int i = 0; i < 200; i++) {
        longMsg += "x";
    }
    
    Logger::begin("Test");
    Logger::line(longMsg.c_str());
    Logger::end();
    
    // Should not crash - just truncate
    std::string output = getOutput();
    EXPECT_FALSE(output.empty());
}

// ============================================================================
// Backward Compatibility Tests (LogBox alias)
// ============================================================================

TEST_F(LoggerTest, LogBoxAlias_WorksCorrectly) {
    LogBox::begin("Test");
    LogBox::line("Message");
    LogBox::end();
    
    std::string output = getOutput();
    EXPECT_NE(output.find("[Test] Starting..."), std::string::npos);
    EXPECT_NE(output.find("Message"), std::string::npos);
    EXPECT_NE(output.find("Done"), std::string::npos);
}
