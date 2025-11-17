#include <src/ui/ui_status.h>
#include <src/ui/screen.h>
#include <src/config.h>

UIStatus::UIStatus(DisplayManager* display) 
    : UIBase(display) {
}

void UIStatus::showAPModeSetup(const char* apName, const char* apIP, const char* mdnsHostname, float batteryVoltage) {
    Screen screen(displayManager, overlayManager, batteryVoltage);
    
    screen.addHeading1("Setup - Step 1");
    
#if !DISPLAY_MINIMAL_UI
    screen.addSpacing(LINE_SPACING);
    screen.addHeading2("Connect WiFi");
#endif
    
    screen.addSpacing(LINE_SPACING);
    screen.addText("1. Connect to WiFi:");
    screen.addText(String("   ") + apName);
    screen.addSpacing(LINE_SPACING);
    
    screen.addText("2. Open browser to:");
    
    // Show mDNS hostname if available
    if (mdnsHostname != nullptr && strlen(mdnsHostname) > 0) {
        String mdnsUrl = "   http://" + String(mdnsHostname);
        screen.addText(mdnsUrl);
        String ipUrl = "   or http://" + String(apIP);
        screen.addText(ipUrl);
    } else {
        String url = "   http://" + String(apIP);
        screen.addText(url);
    }
    
    screen.addSpacing(LINE_SPACING);
    screen.addText("3. Enter WiFi settings");
    
    screen.display();
}

void UIStatus::showConfigModeSetup(const char* localIP, bool hasTimeout, int timeoutMinutes, const char* mdnsHostname, float batteryVoltage) {
    Screen screen(displayManager, overlayManager, batteryVoltage);
    
    screen.addHeading1("Config Mode Active");
    screen.addSpacing(LINE_SPACING * 2);
    screen.addText("Open browser to:");
    
    // Show mDNS hostname if available
    if (mdnsHostname != nullptr && strlen(mdnsHostname) > 0) {
        String mdnsUrl = "   http://" + String(mdnsHostname);
        screen.addHeading2(mdnsUrl);
        String ipUrl = "   or http://" + String(localIP);
        screen.addText(ipUrl);
    } else {
        String url = "   http://" + String(localIP);
        screen.addHeading2(url);
        screen.addSpacing(LINE_SPACING);
    }
    
    if (hasTimeout) {
        String timeoutMsg = "Timeout: " + String(timeoutMinutes) + " minutes";
        screen.addText(timeoutMsg);
    }
    
    screen.display();
}

void UIStatus::showConfigModePartialSetup(const char* localIP, const char* mdnsHostname, float batteryVoltage) {
    Screen screen(displayManager, overlayManager, batteryVoltage);
    
    screen.addHeading1("Setup - Step 2");
    screen.addSpacing(LINE_SPACING * 2);
    screen.addText("Open browser to:");
    
    // Show mDNS hostname if available
    if (mdnsHostname != nullptr && strlen(mdnsHostname) > 0) {
        String mdnsUrl = "   http://" + String(mdnsHostname);
        screen.addHeading2(mdnsUrl);
        String ipUrl = "   or http://" + String(localIP);
        screen.addText(ipUrl);
    } else {
        String url = "   http://" + String(localIP);
        screen.addHeading2(url);
    }
    
    screen.display();
}

void UIStatus::showConfigModeConnecting(const char* ssid, bool isPartialConfig, float batteryVoltage) {
    Screen screen(displayManager, overlayManager, batteryVoltage);
    
    if (isPartialConfig) {
        screen.addHeading1("Setup - Step 2");
        screen.addSpacing(LINE_SPACING * 2);
    } else {
        screen.addHeading1("Config Mode");
        screen.addSpacing(LINE_SPACING);
        screen.addText("Active for 5 minutes");
        screen.addSpacing(LINE_SPACING);
    }
    
    screen.addText("Connecting to:");
    screen.addText(String("   ") + ssid);
    
    screen.display();
}

void UIStatus::showConfigModeWiFiFailed(const char* ssid, float batteryVoltage) {
    Screen screen(displayManager, overlayManager, batteryVoltage);
    
    screen.addHeading1("WiFi Failed");
    screen.addSpacing(LINE_SPACING);
    screen.addText("Cannot connect to:");
    screen.addText(ssid);
    screen.addSpacing(LINE_SPACING);
    screen.addText("Starting AP mode...");
    
    screen.display();
}

void UIStatus::showConfigModeAPFallback(const char* apName, const char* apIP, bool hasTimeout, int timeoutMinutes, const char* mdnsHostname, float batteryVoltage) {
    Screen screen(displayManager, overlayManager, batteryVoltage);
    
    screen.addHeading1("Config Mode (AP)");
    screen.addSpacing(LINE_SPACING);
    screen.addText("WiFi connection failed");
    screen.addSpacing(LINE_SPACING);
    
    screen.addText("Connect to WiFi:");
    screen.addText(String("   ") + apName);
    screen.addSpacing(LINE_SPACING);
    
    screen.addText("Open browser to:");
    
    // Show mDNS hostname if available
    if (mdnsHostname != nullptr && strlen(mdnsHostname) > 0) {
        String mdnsUrl = "   http://" + String(mdnsHostname);
        screen.addText(mdnsUrl);
        String ipUrl = "   or http://" + String(apIP);
        screen.addText(ipUrl);
    } else {
        String url = "   http://" + String(apIP);
        screen.addText(url);
    }
    
    if (hasTimeout) {
        screen.addSpacing(LINE_SPACING);
        String timeoutMsg = "Timeout: " + String(timeoutMinutes) + " minutes";
        screen.addText(timeoutMsg);
    }
    
    screen.display();
}

void UIStatus::showConfigModeTimeout(float batteryVoltage) {
    Screen screen(displayManager, overlayManager, batteryVoltage);
    
    screen.addHeading2("Config Mode Timeout");
    screen.addSpacing(LINE_SPACING);
    screen.addText("Going to sleep...");
    
    screen.display();
}

void UIStatus::showManualRefresh(float batteryVoltage) {
    Screen screen(displayManager, overlayManager, batteryVoltage);
    
    screen.addHeading1("Manual Refresh");
    screen.addSpacing(LINE_SPACING);
    screen.addText("Button pressed - updating...");
    
    screen.display();
}

void UIStatus::showWiFiConfigured(float batteryVoltage) {
    Screen screen(displayManager, overlayManager, batteryVoltage);
    
    screen.addHeading1("WiFi Configured!");
    screen.addSpacing(LINE_SPACING * 2);
    screen.addText("Restarting...");
    
    screen.display();
}

void UIStatus::showSettingsUpdated(float batteryVoltage) {
    Screen screen(displayManager, overlayManager, batteryVoltage);
    
    screen.addHeading1("Settings Updated!");
    screen.addSpacing(LINE_SPACING * 2);
    screen.addText("Restarting...");
    
    screen.display();
}
