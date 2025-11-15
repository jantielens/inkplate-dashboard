/**
 * Test Helpers for Integration Tests
 * 
 * Provides fluent builders and utilities for creating test configurations
 * and scenarios for integration testing.
 */

#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include "config.h"
#include <ctime>

/**
 * Fluent builder for DashboardConfig objects
 * 
 * Example usage:
 *   DashboardConfig config = ConfigBuilder()
 *       .singleImage("http://example.com/image.png", 15)
 *       .withCRC32(true)
 *       .build();
 */
class ConfigBuilder {
private:
    DashboardConfig config;
    
public:
    ConfigBuilder() {
        config = DashboardConfig();
    }
    
    /**
     * Create single image configuration
     * @param url Image URL
     * @param intervalMinutes Update interval in minutes
     */
    ConfigBuilder& singleImage(const char* url, int intervalMinutes) {
        config.imageCount = 1;
        config.imageUrls[0] = String(url);
        config.imageIntervals[0] = intervalMinutes;
        config.imageStay[0] = false;
        return *this;
    }
    
    /**
     * Start building a carousel configuration
     * Call addImage() multiple times to add images
     */
    ConfigBuilder& carousel() {
        config.imageCount = 0;  // Will increment with addImage()
        return *this;
    }
    
    /**
     * Add image to carousel configuration
     * @param url Image URL
     * @param intervalMinutes Update interval in minutes
     * @param stay Whether to stay on this image or auto-advance
     */
    ConfigBuilder& addImage(const char* url, int intervalMinutes, bool stay) {
        if (config.imageCount >= MAX_IMAGE_SLOTS) {
            return *this;  // Ignore if full
        }
        config.imageUrls[config.imageCount] = String(url);
        config.imageIntervals[config.imageCount] = intervalMinutes;
        config.imageStay[config.imageCount] = stay;
        config.imageCount++;
        return *this;
    }
    
    /**
     * Enable or disable CRC32 checking
     */
    ConfigBuilder& withCRC32(bool enabled) {
        config.useCRC32Check = enabled;
        return *this;
    }
    
    /**
     * Configure hourly schedule (inclusive range)
     * @param startHour Start hour (0-23)
     * @param endHour End hour (0-23)
     */
    ConfigBuilder& withHourlySchedule(uint8_t startHour, uint8_t endHour) {
        // Clear all hours first
        config.updateHours[0] = 0x00;
        config.updateHours[1] = 0x00;
        config.updateHours[2] = 0x00;
        
        // Enable hours in range
        for (uint8_t hour = startHour; hour <= endHour; hour++) {
            uint8_t byteIndex = hour / 8;
            uint8_t bitIndex = hour % 8;
            config.updateHours[byteIndex] |= (1 << bitIndex);
        }
        return *this;
    }
    
    /**
     * Set timezone offset
     * @param offset Offset from UTC in hours (e.g., -5 for EST)
     */
    ConfigBuilder& withTimezone(int offset) {
        config.timezoneOffset = offset;
        return *this;
    }
    
    /**
     * Build and return the configuration
     */
    DashboardConfig build() {
        return config;
    }
};

/**
 * Create time_t from human-readable date/time
 * 
 * @param year Year (e.g., 2025)
 * @param month Month (1-12)
 * @param day Day of month (1-31)
 * @param hour Hour (0-23)
 * @param min Minute (0-59)
 * @param sec Second (0-59)
 * @return time_t value representing the specified time
 */
inline time_t createTime(int year, int month, int day, int hour, int min, int sec) {
    struct tm timeinfo = {};
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = min;
    timeinfo.tm_sec = sec;
    timeinfo.tm_isdst = -1;  // Let mktime determine DST
    return mktime(&timeinfo);
}

#endif // TEST_HELPERS_H
