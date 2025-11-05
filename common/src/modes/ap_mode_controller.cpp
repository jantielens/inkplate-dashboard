#include <src/modes/ap_mode_controller.h>

APModeController::APModeController(WiFiManager* wifi, ConfigPortal* portal, UIStatus* uiStatus, UIError* uiError)
    : wifiManager(wifi), configPortal(portal), uiStatus(uiStatus), uiError(uiError) {
}

bool APModeController::begin() {
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
            LogBox::message("Configuration Portal", "Failed to start configuration portal!");
            uiError->showPortalError();
            return false;
        }
    } else {
        LogBox::message("Access Point", "Failed to start Access Point!");
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
