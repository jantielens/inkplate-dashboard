// Mock config_manager.h for unit testing
// This prevents the real config_manager.h from being included
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

// ConfigManager mock is already defined in config.h
// This file exists only to prevent including the real config_manager.h
// which has Arduino dependencies (Preferences.h)

#include "config.h"

#endif // CONFIG_MANAGER_H
