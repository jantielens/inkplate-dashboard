#include <src/ui/ui_messages.h>
#include <src/ui/screen.h>
#include <src/config.h>
#include <src/version.h>

UIMessages::UIMessages(DisplayManager* display) 
    : UIBase(display) {
}

int UIMessages::showHeading(const char* text, int startY, bool clearFirst) {
    if (clearFirst) {
        displayManager->clear();
    }
    displayManager->showMessage(text, MARGIN, startY, FONT_HEADING1);
    return startY + displayManager->getFontHeight(FONT_HEADING1);
}

int UIMessages::showSubheading(const char* text, int currentY) {
    displayManager->showMessage(text, MARGIN, currentY, FONT_HEADING2);
    return currentY + displayManager->getFontHeight(FONT_HEADING2);
}

int UIMessages::showNormalText(const char* text, int currentY, bool indent) {
    int x = indent ? INDENT_MARGIN : MARGIN;
    displayManager->showMessage(text, x, currentY, FONT_NORMAL);
    return currentY + displayManager->getFontHeight(FONT_NORMAL);
}

int UIMessages::showTextWithSpacing(const char* text, int currentY, int extraSpacing) {
    displayManager->showMessage(text, MARGIN, currentY, FONT_NORMAL);
    return currentY + displayManager->getFontHeight(FONT_NORMAL) + (LINE_SPACING * extraSpacing);
}

int UIMessages::addLineSpacing(int currentY, int multiplier) {
    return currentY + (LINE_SPACING * multiplier);
}

void UIMessages::showSplashScreen(const char* boardName, int width, int height, float batteryVoltage) {
    Screen screen(displayManager, overlayManager);
    screen.withLogo().withBattery(batteryVoltage);
    
    screen.addHeading1("Inkplate Dashboard");
    screen.addSpacing(LINE_SPACING);
    screen.addText("github.com/jantielens/inkplate-dashboard");
    screen.addSpacing(LINE_SPACING);
    
    String versionInfo = String(boardName) + " - v" + String(FIRMWARE_VERSION);
    screen.addText(versionInfo);
    
    screen.display();
}

void UIMessages::showConfigInitError() {
    Screen screen(displayManager, overlayManager);
    screen.withLogo();  // Now has logo!
    
    screen.addText("ERROR: Config Init Failed");
    
    screen.display();
}
