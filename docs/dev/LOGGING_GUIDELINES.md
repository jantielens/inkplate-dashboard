# Logging Guidelines for Contributors

This guide explains how to use the Logger class effectively in the Inkplate Dashboard firmware.

## Quick Reference

```cpp
// Basic usage
Logger::begin("Module Name");
Logger::line("Simple message");
Logger::linef("Formatted: %d items", count);
Logger::end(); // Shows "Done (Xms)"

// With custom end message
Logger::end("Custom completion message");

// Nested operations (up to 3 levels)
Logger::begin("Outer Operation");
  Logger::line("Step 1");
  Logger::begin("Inner Operation");
    Logger::line("Sub-step A");
  Logger::end(); // Shows inner timing
  Logger::line("Step 2");
Logger::end(); // Shows outer timing

// Convenience method (begin + line + end in one call)
Logger::message("Quick Log", "Single line message");
Logger::messagef("Quick Log", "Value: %d", 42);
```

## Output Format

The Logger uses indentation to show nested operations:

```
[WiFi] Starting...
  SSID: MyNetwork
  [Channel Lock] Starting...
    Using channel 6
  Done (45ms)
  Connected!
Done (1234ms)
```

Each level of nesting adds 2 spaces of indentation.

## Best Practices

### 1. Use Nesting for Related Operations

Nested logging helps show which operations are part of a larger process:

✅ **Good:**
```cpp
Logger::begin("Image Download");
Logger::line("URL: " + url);

Logger::begin("HTTP Request");
Logger::linef("Attempt %d/%d", attempt, maxRetries);
Logger::end();

Logger::begin("Image Rendering");
Logger::line("Processing bitmap data");
Logger::end();

Logger::end("Download complete");
```

❌ **Bad:**
```cpp
Logger::message("Starting image download", "URL: " + url);
Logger::message("HTTP Request", "Attempt 1/3");
Logger::message("Image Rendering", "Processing bitmap data");
Logger::message("Image Download", "Complete");
```

### 2. Keep Messages Concise

Combine related information into single lines:

✅ **Good:**
```cpp
Logger::linef("Connected! IP: %s, RSSI: %d dBm", ip, rssi);
Logger::linef("CRC32 check: 0x%08X (stored: 0x%08X)", new, stored);
```

❌ **Bad:**
```cpp
Logger::line("Successfully connected to WiFi network!");
Logger::line("IP Address: " + ip);
Logger::line("Signal Strength: " + String(rssi) + " dBm");
```

### 3. Use Short, Clear Labels

Keep prefixes and labels brief but meaningful:

✅ **Good:**
```cpp
Logger::linef("New: 0x%08X", newCRC32);
Logger::linef("Stored: 0x%08X", storedCRC32);
Logger::linef("Retry %d/%d", retry, max);
```

❌ **Bad:**
```cpp
Logger::linef("Newly fetched CRC32 value from remote server: 0x%08X", newCRC32);
Logger::linef("Previously stored CRC32 value in NVS flash: 0x%08X", storedCRC32);
Logger::linef("Retry attempt number %d out of maximum %d attempts", retry, max);
```

### 4. Avoid Redundant Context

The module name is already in `begin()`, don't repeat it:

✅ **Good:**
```cpp
Logger::begin("WiFi Connection");
Logger::line("Scanning networks...");
Logger::line("Found 5 networks");
```

❌ **Bad:**
```cpp
Logger::begin("WiFi Connection");
Logger::line("WiFi: Scanning networks...");
Logger::line("WiFi: Found 5 networks");
```

### 5. Log State Changes and Failures, Not Routine Steps

Focus logging on what went wrong or what's changing:

✅ **Good:**
```cpp
if (!connected) {
    Logger::linef("Connection failed (attempt %d/%d)", attempt, max);
    Logger::linef("Error: %s", getErrorDesc(state));
}
```

❌ **Bad:**
```cpp
Logger::line("Initializing connection...");
Logger::line("Setting up socket...");
Logger::line("Configuring parameters...");
Logger::line("Establishing handshake...");
Logger::line("Connection successful!");
```

### 6. Use Printf-Style Formatting

Prefer `linef()` over String concatenation for better performance:

✅ **Good:**
```cpp
Logger::linef("Temperature: %.1f°C, Humidity: %d%%", temp, humidity);
Logger::linef("Status: %s (%d/100)", status, progress);
```

❌ **Bad:**
```cpp
Logger::line("Temperature: " + String(temp) + "°C, Humidity: " + String(humidity) + "%");
Logger::line("Status: " + status + " (" + String(progress) + "/100)");
```

### 7. Respect the 3-Level Nesting Limit

Maximum nesting depth is 3 levels. Beyond that, timing shows as 0ms:

✅ **Good (3 levels):**
```cpp
Logger::begin("Main Operation");          // Level 0
  Logger::begin("Sub-operation A");        // Level 1
    Logger::begin("Sub-sub-operation");    // Level 2
      Logger::line("Deep work");           // Still tracked
    Logger::end();                         // Shows timing
  Logger::end();
Logger::end();
```

⚠️ **Degraded (4+ levels):**
```cpp
Logger::begin("Level 0");
  Logger::begin("Level 1");
    Logger::begin("Level 2");
      Logger::begin("Level 3");      // Timing = 0ms
        Logger::line("Too deep");
      Logger::end();                 // Shows "Done (0ms)"
    Logger::end();
  Logger::end();
Logger::end();
```

If you need deeper nesting, restructure your code or reduce nesting by logging only critical operations.

## Common Patterns

### Pattern 1: Operation with Retry Logic

```cpp
Logger::begin("Network Request");
for (int attempt = 0; attempt < maxRetries; attempt++) {
    Logger::linef("Attempt %d/%d", attempt + 1, maxRetries);
    
    if (tryRequest()) {
        Logger::end("Success");
        return true;
    }
    
    if (attempt < maxRetries - 1) {
        Logger::linef("Failed, retrying in %dms", retryDelay);
        delay(retryDelay);
    }
}
Logger::end("All attempts failed");
return false;
```

### Pattern 2: Multi-Step Process

```cpp
Logger::begin("System Initialization");

Logger::begin("WiFi Setup");
setupWiFi();
Logger::end();

Logger::begin("NTP Sync");
syncTime();
Logger::end();

Logger::begin("Sensor Init");
initSensors();
Logger::end();

Logger::end("Initialization complete");
```

### Pattern 3: Conditional Operations

```cpp
Logger::begin("Configuration");

if (config.mqttEnabled) {
    Logger::begin("MQTT Setup");
    setupMQTT();
    Logger::end();
} else {
    Logger::line("MQTT disabled - skipping");
}

if (config.otaEnabled) {
    Logger::begin("OTA Check");
    checkForUpdates();
    Logger::end();
}

Logger::end();
```

## Buffer Size Limitation

Logger uses a 128-byte buffer for formatted strings. Very long messages will be truncated.

✅ **Safe:**
```cpp
Logger::linef("Config: mode=%d, interval=%d, url=%s", 
    mode, interval, url.substring(0, 50).c_str());
```

❌ **May truncate:**
```cpp
Logger::linef("Config: mode=%d, interval=%d, url=%s, wifi=%s, mqtt=%s, ...", 
    mode, interval, veryLongUrl.c_str(), ssid.c_str(), broker.c_str(), ...);
```

## When NOT to Log

Avoid logging in these scenarios:

1. **Hot loops**: Don't log inside tight loops that run frequently
2. **Every successful operation**: Only log failures, retries, or state changes
3. **Sensitive data**: Never log passwords, tokens, or personal information
4. **High-frequency events**: Don't log every sensor reading, only summaries

## Testing Your Logs

Always test your logging output by running the firmware and checking:

1. **Indentation is correct**: Each nested level should be 2 spaces deeper
2. **Timing makes sense**: End messages should show reasonable durations
3. **Messages are readable**: Scan the output - is it easy to understand?
4. **No spam**: Make sure you're not logging too much during normal operation

## Example: Good Logging

Here's an example of well-structured logging:

```cpp
void connectAndPublish() {
    Logger::begin("MQTT Publish");
    
    if (!mqttClient.connected()) {
        Logger::begin("Reconnecting");
        for (int i = 0; i < 3; i++) {
            Logger::linef("Attempt %d/3", i + 1);
            if (mqttClient.connect()) {
                Logger::end("Connected");
                break;
            }
            if (i < 2) delay(1000);
        }
        
        if (!mqttClient.connected()) {
            Logger::end("Connection failed");
            return;
        }
    }
    
    Logger::begin("Publishing Data");
    Logger::linef("Topic: %s", topic);
    
    if (mqttClient.publish(topic, payload)) {
        Logger::end("Published");
    } else {
        Logger::end("Publish failed");
    }
    
    Logger::end();
}
```

Output:
```
[MQTT Publish] Starting...
  [Reconnecting] Starting...
    Attempt 1/3
  Connected (1234ms)
  [Publishing Data] Starting...
    Topic: home/sensor/temperature
  Published (56ms)
Done (1345ms)
```

## Summary

- Use `Logger::begin()` to start a log block
- Use `Logger::line()` or `Logger::linef()` to add content
- Use `Logger::end()` to finish the block and show timing
- Nest up to 3 levels deep for hierarchical operations
- Keep messages concise and combine related information
- Focus on failures, retries, and state changes
- Test your output to ensure it's readable and useful
