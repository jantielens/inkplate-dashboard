#include <battery_logic.h>

int calculateBatteryPercentage(float voltage) {
    // Lithium-ion discharge curve for ESP32 devices
    // Based on typical Li-ion/LiPo battery discharge characteristics
    // Voltage range: 4.2V (100%) to 3.0V (0%)
    
    // Voltage to percentage mapping (non-linear discharge curve)
    // These values represent the typical discharge curve of a lithium-ion battery
    const float voltageMap[][2] = {
        {4.20, 100},  // Fully charged
        {4.15, 95},
        {4.11, 90},
        {4.08, 85},
        {4.02, 80},
        {3.98, 75},
        {3.95, 70},
        {3.91, 65},
        {3.87, 60},
        {3.85, 55},
        {3.84, 50},
        {3.82, 45},
        {3.80, 40},
        {3.79, 35},
        {3.77, 30},
        {3.75, 25},
        {3.73, 20},
        {3.71, 15},
        {3.69, 10},
        {3.61, 5},
        {3.00, 0}     // Empty/cutoff voltage
    };
    
    const int mapSize = sizeof(voltageMap) / sizeof(voltageMap[0]);
    
    // Handle out-of-range values
    if (voltage >= voltageMap[0][0]) {
        return 100;  // Above maximum voltage
    }
    if (voltage <= voltageMap[mapSize - 1][0]) {
        return 0;  // Below minimum voltage
    }
    
    // Find the two points to interpolate between
    for (int i = 0; i < mapSize - 1; i++) {
        if (voltage >= voltageMap[i + 1][0] && voltage <= voltageMap[i][0]) {
            // Linear interpolation between two points
            float v1 = voltageMap[i][0];
            float p1 = voltageMap[i][1];
            float v2 = voltageMap[i + 1][0];
            float p2 = voltageMap[i + 1][1];
            
            float percentage = p1 + (voltage - v1) * (p2 - p1) / (v2 - v1);
            
            // Round to nearest 5%
            int rounded = ((int)(percentage + 2.5) / 5) * 5;
            
            // Clamp to valid range
            if (rounded < 0) rounded = 0;
            if (rounded > 100) rounded = 100;
            
            return rounded;
        }
    }
    
    // Fallback (shouldn't reach here)
    return 0;
}
