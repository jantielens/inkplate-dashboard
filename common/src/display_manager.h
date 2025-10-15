#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "Inkplate.h"

class DisplayManager {
public:
    DisplayManager(Inkplate* display);
    
    void init(bool clearOnInit = true);
    void clear();
    void refresh(bool includeVersion = true);
    void showMessage(const char* message, int x, int y, int textSize);
    void drawCentered(const char* message, int y, int textSize);
    
    // Board-specific scaling helpers
    int scaleFont(int baseSize);
    int scaleY(int baseY);
    
    // Board-specific adaptations can be added here
    int getWidth();
    int getHeight();
    
private:
    Inkplate* _display;
    void drawVersionLabel();
};

#endif // DISPLAY_MANAGER_H
