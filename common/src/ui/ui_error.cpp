#include <src/ui/ui_error.h>
#include <src/config.h>

UIError::UIError(DisplayManager* display) 
    : displayManager(display) {
}

void UIError::showWiFiError(const char* ssid, const char* status, int refreshMinutes) {
    // Enable rotation for essential error screen
    displayManager->enableRotation();
    
    displayManager->clear();
    int y = MARGIN;
    
    displayManager->showMessage("WiFi Error!", MARGIN, y, FONT_HEADING1);
    y += displayManager->getFontHeight(FONT_HEADING1) + LINE_SPACING * 2;
    
    displayManager->showMessage("Failed to connect", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    
    String ssidMsg = "SSID: " + String(ssid);
    displayManager->showMessage(ssidMsg.c_str(), MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    
    String statusMsg = "Status: " + String(status);
    displayManager->showMessage(statusMsg.c_str(), MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING * 2;
    
    displayManager->showMessage("Will retry on next wake", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    
    String retryMsg = "(" + String(refreshMinutes) + " minutes)";
    displayManager->showMessage(retryMsg.c_str(), MARGIN, y, FONT_NORMAL);
    
    displayManager->refresh();
}

void UIError::showImageError(const char* url, const char* error, int refreshMinutes) {
    // Enable rotation for essential error screen
    displayManager->enableRotation();
    
    displayManager->clear();
    int y = MARGIN;
    
    displayManager->showMessage("Image Error!", MARGIN, y, FONT_HEADING1);
    y += displayManager->getFontHeight(FONT_HEADING1) + LINE_SPACING * 2;
    
    displayManager->showMessage("Failed to display", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    
    displayManager->showMessage(url, MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    
    String errorMsg = "Error: " + String(error);
    displayManager->showMessage(errorMsg.c_str(), MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING * 2;
    
    displayManager->showMessage("Will retry on next wake", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    
    String retryMsg = "(" + String(refreshMinutes) + " minutes)";
    displayManager->showMessage(retryMsg.c_str(), MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING * 2;
    
    displayManager->showMessage("If you want to update the", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    displayManager->showMessage("Image URL, hold the button", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    displayManager->showMessage("to enter config mode.", MARGIN, y, FONT_NORMAL);
    
    displayManager->refresh();
}

void UIError::showAPStartError() {
    // Enable rotation for essential error screen
    displayManager->enableRotation();
    
    int y = 300;
    displayManager->showMessage("ERROR: AP Start Failed", MARGIN, y, FONT_NORMAL);
    displayManager->refresh();
}

void UIError::showPortalError() {
    // Enable rotation for essential error screen
    displayManager->enableRotation();
    
    int y = 400;
    displayManager->showMessage("ERROR: Portal Failed", MARGIN, y, FONT_NORMAL);
    displayManager->refresh();
}

void UIError::showConfigLoadError() {
    // Enable rotation for essential error screen
    displayManager->enableRotation();
    
    int y = 240;
    displayManager->showMessage("ERROR: Config Load Failed", MARGIN, y, FONT_NORMAL);
    displayManager->refresh();
}

void UIError::showConfigModeFailure() {
    // Enable rotation for essential error screen
    displayManager->enableRotation();
    
    displayManager->clear();
    int y = MARGIN;
    
    displayManager->showMessage("Config Mode Failed", MARGIN, y, FONT_HEADING1);
    y += displayManager->getFontHeight(FONT_HEADING1) + LINE_SPACING * 2;
    
    displayManager->showMessage("Cannot start AP", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING * 2;
    
    displayManager->showMessage("Going to sleep...", MARGIN, y, FONT_NORMAL);
    
    displayManager->refresh();
}
