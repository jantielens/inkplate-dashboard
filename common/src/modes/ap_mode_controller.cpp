#include <src/modes/ap_mode_controller.h>

APModeController::APModeController(WiFiManager* wifi, ConfigPortal* portal, UIStatus* uiStatus, UIError* uiError)
    : wifiManager(wifi), configPortal(portal), uiStatus(uiStatus), uiError(uiError) {
}

bool APModeController::begin() {
    uiStatus->showAPModeStarting();
    
    if (wifiManager->startAccessPoint()) {
        String apName = wifiManager->getAPName();
        String apIP = wifiManager->getAPIPAddress();
        
        uiStatus->showAPModeSetup(apName.c_str(), apIP.c_str());
        
        // Start configuration portal in BOOT_MODE
        if (configPortal->begin(BOOT_MODE)) {
            LogBox::begin("Configuration Portal Active (Boot Mode)");
            LogBox::line("1. Connect to WiFi: " + apName);
            LogBox::line("2. Open: http://" + apIP);
            LogBox::line("3. Enter WiFi credentials");
            LogBox::end();
            return true;
        } else {
            LogBox::begin("Configuration Portal");
            LogBox::line("Failed to start configuration portal!");
            LogBox::end();
            uiError->showPortalError();
            return false;
        }
    } else {
        LogBox::begin("Access Point");
        LogBox::line("Failed to start Access Point!");
        LogBox::end();
        uiError->showAPStartError();
        return false;
    }
}

void APModeController::handleClient() {
    configPortal->handleClient();
}

bool APModeController::isConfigReceived() {
    return configPortal->isConfigReceived();
}
