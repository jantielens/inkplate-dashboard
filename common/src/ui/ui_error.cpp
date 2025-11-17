#include <src/ui/ui_error.h>
#include <src/ui/screen.h>
#include <src/config.h>

UIError::UIError(DisplayManager* display) 
    : UIBase(display) {
}

void UIError::showWiFiError(const char* ssid, const char* status, float batteryVoltage) {
    Screen screen(displayManager, overlayManager);
    screen.withLogo().withBattery(batteryVoltage);
    
    screen.addHeading1("WiFi Error!");
    screen.addSpacing(LINE_SPACING);
    
    String ssidMsg = "SSID: " + String(ssid);
    screen.addText(ssidMsg);
    
    String statusMsg = "Status: " + String(status);
    screen.addText(statusMsg);
    screen.addSpacing(LINE_SPACING);
    
    screen.addText("Failed to connect to WiFi.");
    screen.addSpacing(LINE_SPACING);
    
    screen.addText("Trying again in 1 minute");
    screen.addText("(or press button).");
    screen.addText("Hold button to enter config mode.");
    
    screen.display();
}

void UIError::showImageError(const char* url, const char* error, float batteryVoltage) {
    Screen screen(displayManager, overlayManager);
    screen.withLogo().withBattery(batteryVoltage);
    
    screen.addHeading1("Image Error!");
    screen.addSpacing(LINE_SPACING);
    
    // Show URL with extra space for potential wrapping
    screen.addText(url);
    screen.addSpacing(LINE_SPACING * 2);
    
    screen.addText("Failed to download or draw image.");
    screen.addSpacing(LINE_SPACING);
    
    screen.addText("Trying again in 1 minute");
    screen.addText("(or press button).");
    screen.addText("Hold button to enter config mode.");
    
    screen.display();
}

void UIError::showAPStartError(float batteryVoltage) {
    Screen screen(displayManager, overlayManager);
    screen.withLogo().withBattery(batteryVoltage);
    
    screen.addText("ERROR: AP Start Failed");
    
    screen.display();
}

void UIError::showPortalError(float batteryVoltage) {
    Screen screen(displayManager, overlayManager);
    screen.withLogo().withBattery(batteryVoltage);
    
    screen.addText("ERROR: Portal Failed");
    
    screen.display();
}

void UIError::showConfigLoadError(float batteryVoltage) {
    Screen screen(displayManager, overlayManager);
    screen.withLogo().withBattery(batteryVoltage);
    
    screen.addText("ERROR: Config Load Failed");
    
    screen.display();
}

void UIError::showConfigModeFailure(float batteryVoltage) {
    Screen screen(displayManager, overlayManager);
    screen.withLogo().withBattery(batteryVoltage);
    
    screen.addHeading1("Config Mode Failed");
    screen.addSpacing(LINE_SPACING);
    screen.addText("Cannot start AP");
    screen.addSpacing(LINE_SPACING);
    screen.addText("Going to sleep...");
    
    screen.display();
}
