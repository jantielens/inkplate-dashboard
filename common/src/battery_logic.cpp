#include <battery_logic.h>

int calculateBatteryPercentage(float voltage) {
    // Lithium-ion discharge curve for ESP32 devices
    // Based on real-world discharge test (Nov 2025, 192-hour runtime)
    // Voltage range: 4.13V (100%) to 3.43V (0% - device cutoff)
    // Data source: GitHub issue #57 battery discharge test
    
    // Voltage to percentage mapping (non-linear discharge curve)
    // These values represent actual measured discharge behavior
    const float voltageMap[][2] = {
        {4.13, 100},  // Fully charged (real-world maximum)
        {4.12, 95},
        {4.08, 90},
        {4.04, 85},
        {4.02, 80},
        {4.00, 75},
        {3.98, 70},
        {3.95, 65},
        {3.92, 60},
        {3.88, 55},
        {3.85, 50},   // Mid-point
        {3.82, 45},
        {3.79, 40},
        {3.75, 35},
        {3.71, 30},
        {3.66, 25},
        {3.64, 20},   // Low battery warning threshold
        {3.59, 15},
        {3.53, 10},
        {3.47, 5},
        {3.43, 0}     // Device cutoff (stops refreshing)
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
