#ifndef TIMEZONES_H
#define TIMEZONES_H

#include <Arduino.h>

// Timezone mapping structure
struct TimezoneMapping {
    const char* displayName;   // Human-friendly name (e.g., "Europe/Berlin")
    const char* posixTz;       // POSIX TZ string for setenv/tzset
};

// Curated list of common timezones with POSIX TZ strings
// Format: displayName, POSIX TZ string
// POSIX TZ format: std offset [dst [offset][,start[/time],end[/time]]]
// Example: "CET-1CEST,M3.5.0,M10.5.0/3" means:
//   - CET (Central European Time) is UTC+1 (note: sign is inverted in POSIX)
//   - CEST (Central European Summer Time) is UTC+2
//   - DST starts last Sunday in March (M3.5.0)
//   - DST ends last Sunday in October at 3am (M10.5.0/3)
const TimezoneMapping TIMEZONE_MAPPINGS[] = {
    // UTC
    {"UTC", "UTC0"},
    
    // Europe
    {"Europe/London", "GMT0BST,M3.5.0/1,M10.5.0"},
    {"Europe/Paris", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Berlin", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Amsterdam", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Brussels", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Rome", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Madrid", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Vienna", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Zurich", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Athens", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Helsinki", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Stockholm", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Copenhagen", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Oslo", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Warsaw", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Prague", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Budapest", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Bucharest", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Moscow", "MSK-3"},  // No DST since 2014
    {"Europe/Istanbul", "TRT-3"},  // Turkey stopped DST in 2016
    {"Europe/Dublin", "IST-1GMT0,M10.5.0,M3.5.0/1"},  // Irish Standard Time in summer
    {"Europe/Lisbon", "WET0WEST,M3.5.0/1,M10.5.0"},
    
    // North America
    {"America/New_York", "EST5EDT,M3.2.0,M11.1.0"},
    {"America/Chicago", "CST6CDT,M3.2.0,M11.1.0"},
    {"America/Denver", "MST7MDT,M3.2.0,M11.1.0"},
    {"America/Los_Angeles", "PST8PDT,M3.2.0,M11.1.0"},
    {"America/Phoenix", "MST7"},  // Arizona - no DST
    {"America/Anchorage", "AKST9AKDT,M3.2.0,M11.1.0"},
    {"America/Toronto", "EST5EDT,M3.2.0,M11.1.0"},
    {"America/Vancouver", "PST8PDT,M3.2.0,M11.1.0"},
    {"America/Mexico_City", "CST6CDT,M4.1.0,M10.5.0"},
    {"America/Sao_Paulo", "BRT3BRST,M10.3.0/0,M2.3.0/0"},  // Brazil
    {"America/Argentina/Buenos_Aires", "ART3"},  // No DST since 2009
    
    // Asia
    {"Asia/Tokyo", "JST-9"},  // No DST
    {"Asia/Seoul", "KST-9"},  // No DST
    {"Asia/Shanghai", "CST-8"},  // No DST
    {"Asia/Hong_Kong", "HKT-8"},  // No DST
    {"Asia/Singapore", "SGT-8"},  // No DST
    {"Asia/Bangkok", "ICT-7"},  // No DST
    {"Asia/Dubai", "GST-4"},  // No DST
    {"Asia/Kolkata", "IST-5:30"},  // India - no DST
    {"Asia/Karachi", "PKT-5"},  // No DST
    {"Asia/Jerusalem", "IST-2IDT,M3.4.4/26,M10.5.0"},  // Israel DST
    
    // Australia / Pacific
    {"Australia/Sydney", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
    {"Australia/Melbourne", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
    {"Australia/Brisbane", "AEST-10"},  // Queensland - no DST
    {"Australia/Perth", "AWST-8"},  // No DST
    {"Australia/Adelaide", "ACST-9:30ACDT,M10.1.0,M4.1.0/3"},
    {"Pacific/Auckland", "NZST-12NZDT,M9.5.0,M4.1.0/3"},  // New Zealand
    {"Pacific/Fiji", "FJT-12FJST,M11.1.0,M1.3.0/3"},
    {"Pacific/Honolulu", "HST10"},  // Hawaii - no DST
    
    // Africa
    {"Africa/Cairo", "EET-2"},  // Egypt stopped DST in 2016
    {"Africa/Johannesburg", "SAST-2"},  // No DST
    {"Africa/Nairobi", "EAT-3"},  // No DST
    {"Africa/Lagos", "WAT-1"},  // No DST
    
    // Atlantic
    {"Atlantic/Reykjavik", "GMT0"},  // Iceland - no DST
    {"Atlantic/Azores", "AZOT1AZOST,M3.5.0/0,M10.5.0/1"},
};

// Number of timezones in the mapping table
const int TIMEZONE_COUNT = sizeof(TIMEZONE_MAPPINGS) / sizeof(TimezoneMapping);

// Helper function to find POSIX TZ string by display name
// Returns nullptr if not found
const char* findPosixTzByName(const String& displayName) {
    for (int i = 0; i < TIMEZONE_COUNT; i++) {
        if (displayName.equals(TIMEZONE_MAPPINGS[i].displayName)) {
            return TIMEZONE_MAPPINGS[i].posixTz;
        }
    }
    return nullptr;
}

// Helper function to validate timezone name
// Returns true if the timezone exists in the mapping table
bool isValidTimezoneName(const String& displayName) {
    return findPosixTzByName(displayName) != nullptr;
}

#endif // TIMEZONES_H
