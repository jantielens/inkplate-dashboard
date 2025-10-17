#include <src/ui/ui_status.h>
#include <src/config.h>

UIStatus::UIStatus(DisplayManager* display) 
    : displayManager(display) {
}

void UIStatus::showAPModeSetup(const char* apName, const char* apIP) {
    displayManager->clear();
    int y = MARGIN;
    
    displayManager->showMessage("Setup - Step 1", MARGIN, y, FONT_HEADING1);
    y += displayManager->getFontHeight(FONT_HEADING1) + LINE_SPACING * 2;
    
    displayManager->showMessage("Connect WiFi", MARGIN, y, FONT_HEADING2);
    y += displayManager->getFontHeight(FONT_HEADING2) + LINE_SPACING * 2;
    
    displayManager->showMessage("1. Connect to WiFi:", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    displayManager->showMessage(apName, INDENT_MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING * 2;
    
    displayManager->showMessage("2. Open browser to:", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    String url = "http://" + String(apIP);
    displayManager->showMessage(url.c_str(), INDENT_MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING * 2;
    
    displayManager->showMessage("3. Enter WiFi settings", MARGIN, y, FONT_NORMAL);
    
    displayManager->refresh();
}

void UIStatus::showAPModeStarting() {
    int y = 240;
    displayManager->showMessage("Status: First Boot", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    displayManager->showMessage("Starting AP Mode...", MARGIN, y, FONT_NORMAL);
    displayManager->refresh();
}

void UIStatus::showConfigModeSetup(const char* localIP, bool hasTimeout, int timeoutMinutes) {
    displayManager->clear();
    int y = MARGIN;
    
    displayManager->showMessage("Config Mode Active", MARGIN, y, FONT_HEADING1);
    y += displayManager->getFontHeight(FONT_HEADING1) + LINE_SPACING * 3;
    
    displayManager->showMessage("Open browser to:", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    
    String url = "http://" + String(localIP);
    displayManager->showMessage(url.c_str(), INDENT_MARGIN, y, FONT_HEADING2);
    y += displayManager->getFontHeight(FONT_HEADING2) + LINE_SPACING * 2;
    
    if (hasTimeout) {
        String timeoutMsg = "Timeout: " + String(timeoutMinutes) + " minutes";
        displayManager->showMessage(timeoutMsg.c_str(), MARGIN, y, FONT_NORMAL);
    }
    
    displayManager->refresh();
}

void UIStatus::showConfigModePartialSetup(const char* localIP) {
    displayManager->clear();
    int y = MARGIN;
    
    displayManager->showMessage("Setup - Step 2", MARGIN, y, FONT_HEADING1);
    y += displayManager->getFontHeight(FONT_HEADING1) + LINE_SPACING * 3;
    
    displayManager->showMessage("Open browser to:", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    
    String url = "http://" + String(localIP);
    displayManager->showMessage(url.c_str(), INDENT_MARGIN, y, FONT_HEADING2);
    
    displayManager->refresh();
}

void UIStatus::showConfigModeConnecting(const char* ssid, bool isPartialConfig) {
    displayManager->clear();
    int y = MARGIN;
    
    if (isPartialConfig) {
        displayManager->showMessage("Setup - Step 2", MARGIN, y, FONT_HEADING1);
        y += displayManager->getFontHeight(FONT_HEADING1) + LINE_SPACING * 2;
        displayManager->showMessage("Configure Dashboard", MARGIN, y, FONT_HEADING2);
        y += displayManager->getFontHeight(FONT_HEADING2) + LINE_SPACING * 2;
    } else {
        displayManager->showMessage("Config Mode", MARGIN, y, FONT_HEADING1);
        y += displayManager->getFontHeight(FONT_HEADING1) + LINE_SPACING * 2;
        displayManager->showMessage("Active for 5 minutes", MARGIN, y, FONT_NORMAL);
        y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING * 2;
    }
    
    displayManager->showMessage("Connecting to WiFi...", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    displayManager->showMessage(ssid, INDENT_MARGIN, y, FONT_NORMAL);
    
    displayManager->refresh();
}

void UIStatus::showConfigModeWiFiFailed(const char* ssid) {
    displayManager->clear();
    int y = MARGIN;
    
    displayManager->showMessage("WiFi Failed", MARGIN, y, FONT_HEADING1);
    y += displayManager->getFontHeight(FONT_HEADING1) + LINE_SPACING * 2;
    
    displayManager->showMessage("Cannot connect to:", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    displayManager->showMessage(ssid, MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING * 2;
    
    displayManager->showMessage("Starting AP mode...", MARGIN, y, FONT_NORMAL);
    
    displayManager->refresh();
}

void UIStatus::showConfigModeAPFallback(const char* apName, const char* apIP, bool hasTimeout, int timeoutMinutes) {
    displayManager->clear();
    int y = MARGIN;
    
    displayManager->showMessage("Config Mode (AP)", MARGIN, y, FONT_HEADING1);
    y += displayManager->getFontHeight(FONT_HEADING1) + LINE_SPACING * 2;
    
    displayManager->showMessage("WiFi connection failed", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING * 2;
    
    displayManager->showMessage("Connect to WiFi:", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    displayManager->showMessage(apName, INDENT_MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING * 2;
    
    displayManager->showMessage("Open browser to:", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    String url = "http://" + String(apIP);
    displayManager->showMessage(url.c_str(), INDENT_MARGIN, y, FONT_NORMAL);
    
    displayManager->refresh();
}

void UIStatus::showConfigModeTimeout() {
    displayManager->clear();
    int y = MARGIN;
    
    displayManager->showMessage("Config Mode Timeout", MARGIN, y, FONT_HEADING2);
    y += displayManager->getFontHeight(FONT_HEADING2) + LINE_SPACING * 2;
    displayManager->showMessage("Going to sleep...", MARGIN, y, FONT_NORMAL);
    
    displayManager->refresh();
}

void UIStatus::showDebugStatus(const char* ssid, int refreshMinutes) {
    int y = 240;
    displayManager->showMessage("Status: Configured", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    
    String ssidMsg = "SSID: " + String(ssid);
    displayManager->showMessage(ssidMsg.c_str(), MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    
    String refreshMsg = "Refresh: " + String(refreshMinutes) + " min";
    displayManager->showMessage(refreshMsg.c_str(), MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    
    displayManager->showMessage("Connecting to WiFi...", MARGIN, y, FONT_NORMAL);
    displayManager->refresh();
}

void UIStatus::showDownloading(const char* url, bool mqttConnected) {
    displayManager->clear();
    int y = MARGIN;
    
    displayManager->showMessage("Downloading...", MARGIN, y, FONT_HEADING2);
    y += displayManager->getFontHeight(FONT_HEADING2) + LINE_SPACING * 2;
    
    displayManager->showMessage(url, MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    
    if (mqttConnected) {
        displayManager->showMessage("MQTT: Connected", MARGIN, y, FONT_NORMAL);
    }
    displayManager->refresh();
}

void UIStatus::showManualRefresh() {
    displayManager->clear();
    int y = MARGIN;
    
    displayManager->showMessage("Manual Refresh", MARGIN, y, FONT_HEADING1);
    y += displayManager->getFontHeight(FONT_HEADING1) + LINE_SPACING * 2;
    displayManager->showMessage("Button pressed - updating...", MARGIN, y, FONT_NORMAL);
    
    displayManager->refresh();
}

void UIStatus::showWiFiConfigured() {
    displayManager->clear();
    int y = MARGIN;
    
    displayManager->showMessage("WiFi Configured!", MARGIN, y, FONT_HEADING1);
    y += displayManager->getFontHeight(FONT_HEADING1) + LINE_SPACING * 3;
    displayManager->showMessage("Restarting...", MARGIN, y, FONT_NORMAL);
    
    displayManager->refresh();
}

void UIStatus::showSettingsUpdated() {
    displayManager->clear();
    int y = MARGIN;
    
    displayManager->showMessage("Settings Updated!", MARGIN, y, FONT_HEADING1);
    y += displayManager->getFontHeight(FONT_HEADING1) + LINE_SPACING * 3;
    displayManager->showMessage("Restarting...", MARGIN, y, FONT_NORMAL);
    
    displayManager->refresh();
}
