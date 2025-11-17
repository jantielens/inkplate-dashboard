#include <src/modes/ap_mode_controller.h>

APModeController::APModeController(WiFiManager* wifi, ConfigPortal* portal, UIStatus* uiStatus, UIError* uiError)
    : wifiManager(wifi), configPortal(portal), uiStatus(uiStatus), uiError(uiError), powerManager(nullptr), display(nullptr) {
}

void APModeController::setPowerManager(PowerManager* power) {
    powerManager = power;
}

void APModeController::setDisplay(void* disp) {
    display = disp;
}

bool APModeController::begin() {
    if (wifiManager->startAccessPoint()) {
        String apName = wifiManager->getAPName();
        String apIP = wifiManager->getAPIPAddress();
        String mdnsHostname = wifiManager->getMDNSHostname();
        
        float batteryVoltage = 0.0;
        if (powerManager != nullptr && display != nullptr) {
            batteryVoltage = powerManager->readBatteryVoltage(display);
        }
        
        uiStatus->showAPModeSetup(apName.c_str(), apIP.c_str(), mdnsHostname.c_str(), batteryVoltage);
        
        // Start configuration portal in BOOT_MODE
        if (configPortal->begin(BOOT_MODE)) {
            Logger::begin("Configuration Portal Active (Boot Mode)");
            Logger::line("1. Connect to WiFi: " + apName);
            if (mdnsHostname.length() > 0) {
                Logger::line("2. Open: http://" + mdnsHostname + " or http://" + apIP);
            } else {
                Logger::line("2. Open: http://" + apIP);
            }
            Logger::line("3. Enter WiFi credentials");
            Logger::end();
            return true;
        } else {
            Logger::message("Configuration Portal", "Failed to start configuration portal!");
            uiError->showPortalError();
            return false;
        }
    } else {
        Logger::message("Access Point", "Failed to start Access Point!");
        uiError->showAPStartError();
        return false;
    }
}

void APModeController::handleClient() {
    wifiManager->handleDNS();  // Process DNS requests for captive portal
    configPortal->handleClient();
}

bool APModeController::isConfigReceived() {
    return configPortal->isConfigReceived();
}
