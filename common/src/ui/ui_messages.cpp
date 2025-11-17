#include <src/ui/ui_messages.h>
#include <src/config.h>
#include <src/version.h>
#include <src/logo_bitmap.h>

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
    // Enable rotation for splash screen
    // User sees this on first boot or debug mode - should be readable
    displayManager->enableRotation();
    displayManager->clear();

    // Use board-specific MARGIN for logo position
    // Center the logo horizontally within the margins
    int screenWidth = displayManager->getWidth();
    int minLogoX = MARGIN;
    int maxLogoX = screenWidth - LOGO_WIDTH - MARGIN;
    int logoX;
    if (maxLogoX <= minLogoX) {
        // Not enough space to center, use left margin
        logoX = minLogoX;
    } else {
        logoX = minLogoX + (maxLogoX - minLogoX) / 2;
    }
    int logoY = MARGIN;

#if !DISPLAY_MINIMAL_UI
    displayManager->drawBitmap(logo_bitmap, logoX, logoY, LOGO_WIDTH, LOGO_HEIGHT);
    int textStartY = logoY + LOGO_HEIGHT + MARGIN; // spacing below logo uses MARGIN
#else
    int textStartY = logoY;  // Start at top when logo is skipped
#endif

    int y = textStartY;
    y = showHeading("Inkplate Dashboard", y);
    y = addLineSpacing(y, 2);
    y = showNormalText("github.com/jantielens/inkplate-dashboard", y);
    y = addLineSpacing(y, 2);
    String versionInfo = String(boardName) + " - v" + String(FIRMWARE_VERSION);
    y = showNormalText(versionInfo.c_str(), y);
    
    // Draw battery icon at bottom left
    drawBatteryIconBottomLeft(batteryVoltage);
    
    displayManager->refresh();
}

void UIMessages::showConfigInitError() {
    // Enable rotation for essential error screen
    displayManager->enableRotation();
    
    displayManager->clear();
    int y = MARGIN;
    showNormalText("ERROR: Config Init Failed", y);
    displayManager->refresh();
}
