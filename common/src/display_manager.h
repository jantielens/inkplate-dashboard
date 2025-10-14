#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "Inkplate.h"

class DisplayManager {
public:
    DisplayManager(Inkplate* display);
    
    void init();
    void clear();
    void refresh();
    void showMessage(const char* message, int x, int y, int textSize);
    void drawCentered(const char* message, int y, int textSize);
    
    // Board-specific adaptations can be added here
    int getWidth();
    int getHeight();
    
private:
    Inkplate* _display;
};

#endif // DISPLAY_MANAGER_H
