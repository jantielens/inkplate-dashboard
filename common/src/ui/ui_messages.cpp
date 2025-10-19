#include <src/ui/ui_messages.h>
#include <src/config.h>

UIMessages::UIMessages(DisplayManager* display) 
    : displayManager(display) {
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

void UIMessages::showSplashScreen(const char* boardName, int width, int height) {
    // Enable rotation for splash screen
    // User sees this on first boot or debug mode - should be readable
    displayManager->enableRotation();
    
    displayManager->clear();
    
    int y = MARGIN;
    y = showHeading("Dashboard", y);
    y = addLineSpacing(y, 3);
    
    y = showSubheading(boardName, y);
    y = addLineSpacing(y, 2);
    
    String dimensions = String(width) + "x" + String(height);
    y = showNormalText(dimensions.c_str(), y);
}

void UIMessages::showConfigInitError() {
    // Enable rotation for essential error screen
    displayManager->enableRotation();
    
    displayManager->clear();
    int y = 240;
    showNormalText("ERROR: Config Init Failed", y);
    displayManager->refresh();
}
