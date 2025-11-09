#include <src/ui/ui_error.h>
#include <src/config.h>

UIError::UIError(DisplayManager* display) 
    : displayManager(display) {
}

void UIError::showWiFiError(const char* ssid, const char* status) {
    // Enable rotation for essential error screen
    displayManager->enableRotation();
    
    displayManager->clear();
    int y = MARGIN;
    
    displayManager->showMessage("WiFi Error!", MARGIN, y, FONT_HEADING1);
    y += displayManager->getFontHeight(FONT_HEADING1) + LINE_SPACING * 2;
    
    String ssidMsg = "SSID: " + String(ssid);
    displayManager->showMessage(ssidMsg.c_str(), MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    
    String statusMsg = "Status: " + String(status);
    displayManager->showMessage(statusMsg.c_str(), MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING * 2;
    
    displayManager->showMessage("Failed to connect to WiFi.", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING * 2;
    
    displayManager->showMessage("Trying again in 1 minute", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    displayManager->showMessage("(or press button).", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    displayManager->showMessage("Hold button to enter config mode.", MARGIN, y, FONT_NORMAL);
    
    displayManager->refresh();
}

void UIError::showImageError(const char* url, const char* error) {
    // Enable rotation for essential error screen
    displayManager->enableRotation();
    
    displayManager->clear();
    int y = MARGIN;
    
    displayManager->showMessage("Image Error!", MARGIN, y, FONT_HEADING1);
    y += displayManager->getFontHeight(FONT_HEADING1) + LINE_SPACING * 2;
    
    // Show URL with extra space for potential wrapping
    displayManager->showMessage(url, MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) * 2 + LINE_SPACING * 2;
    
    displayManager->showMessage("Failed to download or draw image.", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING * 2;
    
    displayManager->showMessage("Trying again in 1 minute", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    displayManager->showMessage("(or press button).", MARGIN, y, FONT_NORMAL);
    y += displayManager->getFontHeight(FONT_NORMAL) + LINE_SPACING;
    displayManager->showMessage("Hold button to enter config mode.", MARGIN, y, FONT_NORMAL);
    
    displayManager->refresh();
}

void UIError::showAPStartError() {
    // Enable rotation for essential error screen
    displayManager->enableRotation();
    
    displayManager->clear();
    int y = MARGIN;
    displayManager->showMessage("ERROR: AP Start Failed", MARGIN, y, FONT_NORMAL);
    displayManager->refresh();
}

void UIError::showPortalError() {
    // Enable rotation for essential error screen
    displayManager->enableRotation();
    
    displayManager->clear();
    int y = MARGIN;
    displayManager->showMessage("ERROR: Portal Failed", MARGIN, y, FONT_NORMAL);
    displayManager->refresh();
}

void UIError::showConfigLoadError() {
    // Enable rotation for essential error screen
    displayManager->enableRotation();
    
    displayManager->clear();
    int y = MARGIN;
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
