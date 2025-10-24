// Config Portal HTML Templates
// This file contains HTML templates and snippets for the configuration web interface
// These templates are embedded in the firmware to avoid filesystem dependencies.

#ifndef CONFIG_PORTAL_HTML_H
#define CONFIG_PORTAL_HTML_H

// Battery Life Estimator HTML Template
// This is the static HTML structure for the battery life estimator widget
// JavaScript will populate the dynamic values (battery life, power consumption, etc.)
const char* CONFIG_PORTAL_BATTERY_ESTIMATOR_HTML = R"(
<div class='battery-estimator'>
  <div class='battery-estimator-header'>
    <span style='font-size: 24px;'>🔋</span>
    <h3>Battery Life Estimate</h3>
  </div>
  
  <div style='background: #e0f2fe; border-left: 3px solid #0284c7; padding: 10px; border-radius: 4px; margin-bottom: 15px; font-size: 13px; color: #0c4a6e;'>
    <strong>ℹ️ Information Only:</strong> This estimator helps you plan your configuration. Battery capacity and expected image changes are not saved and do not affect device operation.
  </div>
  
  <div class='battery-capacity-input'>
    <label for='battery-capacity'>Battery Capacity</label>
    <select id='battery-capacity'>
      <option value='600'>600 mAh (Small)</option>
      <option value='1200' selected>1200 mAh (Standard)</option>
      <option value='1800'>1800 mAh (Medium)</option>
      <option value='3000'>3000 mAh (Large)</option>
      <option value='6000'>6000 mAh (Extra Large)</option>
    </select>
  </div>
  
  <div class='battery-capacity-input'>
    <label for='daily-changes'>Expected Image Changes Per Day</label>
    <input type='number' id='daily-changes' min='1' max='1000' value='5' style='width: 100%; padding: 10px; border: 2px solid #e0e0e0; border-radius: 6px; font-size: 14px;'>
    <div style='font-size: 11px; color: #666; margin-top: 5px;'>How many times per day does your image content actually change? (Default: 5)</div>
  </div>
  
  <div class='battery-result good' id='battery-result'>
    <div style='display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px;'>
      <div class='battery-life-display' id='battery-days'>174 days</div>
      <span class='status-badge status-good' id='status-badge'>GOOD</span>
    </div>
    <div class='battery-life-subtitle' id='battery-months'>Approximately 5.8 months</div>
    <div class='battery-progress'>
      <div class='battery-progress-bar good' id='progress-bar' style='width: 58%;'></div>
    </div>
    <div class='battery-details'>
      <div class='battery-detail-item'>
        <span class='battery-detail-label'>Daily Power</span>
        <span class='battery-detail-value' id='daily-power'>6.9 mAh</span>
      </div>
      <div class='battery-detail-item'>
        <span class='battery-detail-label'>Wake-ups/Day</span>
        <span class='battery-detail-value' id='wakeups'>288</span>
      </div>
      <div class='battery-detail-item'>
        <span class='battery-detail-label'>Active Time</span>
        <span class='battery-detail-value' id='active-time'>3.4 min/day</span>
      </div>
      <div class='battery-detail-item'>
        <span class='battery-detail-label'>Sleep Time</span>
        <span class='battery-detail-value' id='sleep-time'>23.9 hrs/day</span>
      </div>
    </div>
  </div>
  
  <div class='battery-tips' id='battery-tips' style='display: none;'>
    <strong>💡 Tip:</strong> <span id='tip-text'></span>
  </div>
  
  <div class='assumptions'>
    <div class='assumptions-title'>Estimation assumes:</div>
    <ul>
      <li>WiFi: 100mA average, Display: 50mA, Sleep: 20µA</li>
      <li>7s per image update, 1s per CRC32 check</li>
      <li>Good WiFi signal strength</li>
    </ul>
  </div>
</div>
)";

// Factory Reset Modal HTML Template
// This is the confirmation modal that appears when user clicks factory reset
const char* CONFIG_PORTAL_RESET_MODAL_HTML = R"(
<div id='resetModal' class='modal'>
  <div class='modal-content'>
    <h2 style='color: #dc2626; margin-bottom: 15px;'>⚠️ Confirm Factory Reset</h2>
    <p style='color: #666; font-size: 14px; margin-bottom: 20px;'>This will permanently delete all configuration settings. Are you sure you want to continue?</p>
    <div class='modal-buttons'>
      <button onclick='hideResetModal()'>Cancel</button>
      <form action='/factory-reset' method='POST' style='display: inline;'>
        <button type='submit' class='danger-button'>Yes, Reset Everything</button>
      </form>
    </div>
  </div>
</div>
)";

// Footer Template - use String.replace("%VERSION%", actual_version)
const char* CONFIG_PORTAL_FOOTER_TEMPLATE = R"(
<div style='text-align: center; margin-top: 30px; padding-top: 20px; border-top: 1px solid #e0e0e0; color: #999; font-size: 12px;'>
  Inkplate Dashboard v%VERSION%
</div>
)";

// OTA Page - Static HTML Content
const char* CONFIG_PORTAL_OTA_CONTENT_HTML = R"(
<div class='warning-banner'>
<strong>⚠️ Important:</strong> Do not power off the device during the update process. The device will restart automatically after a successful update.
</div>
<div style='margin-top: 30px; padding: 20px; border: 2px solid #e0e0e0; border-radius: 8px;'>
<h2 style='margin-top: 0;'>📦 Option 1: Update from GitHub Releases</h2>
<p style='color: #666; margin-bottom: 20px;'>Check for and install the latest firmware directly from GitHub.</p>
<div id='checkLoading' style='text-align: center; margin-top: 15px; color: #666;'>
<div style='display: inline-block; margin-right: 10px;'>⏳</div> Checking GitHub...
</div>
<button type='button' id='checkUpdateBtn' class='btn-primary' style='display: none; width: 100%; margin-top: 15px;' onclick='checkForUpdates()'>Retry Check</button>
<div id='updateResults' style='display: none; margin-top: 20px;'>
<div id='updateInfo' style='padding: 15px; background: #f5f5f5; border-radius: 6px; margin-bottom: 15px;'></div>
<button type='button' id='installUpdateBtn' style='display: none; width: 100%;' class='btn-primary' onclick='installUpdate()'>Install Update</button>
</div>
<div id='checkError' style='display: none; margin-top: 15px; padding: 12px; background: #ffe6e6; border-left: 4px solid #cc0000; border-radius: 4px;'>
<strong>Error:</strong> <span id='checkErrorText'></span>
</div>
</div>
<div style='margin-top: 30px; padding: 20px; border: 2px solid #e0e0e0; border-radius: 8px;'>
<h2 style='margin-top: 0;'>📁 Option 2: Upload Local Firmware File</h2>
<p style='color: #666; margin-bottom: 20px;'>Upload a firmware file (.bin) from your computer.</p>
<form id='otaForm' method='POST' action='/ota' enctype='multipart/form-data'>
<div class='form-group'>
<label for='update'>Select Firmware File (.bin)</label>
<input type='file' id='update' name='update' accept='.bin' required>
<div class='help-text'>Choose a .bin firmware file for your device</div>
</div>
<div id='progressSection' style='display: none;'>
<div class='progress-container'>
<div class='progress-bar' id='progressBar'>0%</div>
</div>
<div id='statusText' class='help-text' style='text-align: center; margin-top: 10px;'></div>
</div>
<div style='display: flex; gap: 10px; margin-top: 25px;'>
<button type='submit' id='uploadBtn' class='btn-primary' style='flex: 1;'>Upload & Install</button>
</div>
</form>
</div>
<div id='installProgress' style='display: none; margin-top: 20px; padding: 20px; background: #e8f4f8; border-radius: 8px; text-align: center;'>
<h3 style='margin-top: 0;'>Installing Firmware...</h3>
<p id='installStatus'>Downloading from GitHub...</p>
<div class='progress-container' style='margin-top: 15px;'>
<div class='progress-bar' id='installProgressBar'>0%</div>
</div>
<p style='margin-top: 15px; color: #cc0000;'><strong>⚠️ Do not power off the device!</strong></p>
</div>
<div id='successMessage' class='success' style='display: none; margin-top: 20px;'>
<h2>✅ Update Successful!</h2>
<p>Firmware updated successfully. Device is restarting...</p>
</div>
<div id='errorMessage' class='error' style='display: none; margin-top: 20px;'>
<h2>❌ Update Failed</h2>
<p id='errorText'></p>
</div>
<div style='margin-top: 30px;'>
<button type='button' class='btn-secondary' style='width: 100%;' onclick='window.location.href="/"'>← Back to Configuration</button>
</div>
)";

// Page Header Template - reusable HTML header for all portal pages
// This reduces code duplication across different page generation functions
const char* CONFIG_PORTAL_PAGE_HEADER_START = R"(<!DOCTYPE html><html><head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1.0'>
)";

// OTA Status Page - Custom CSS for spinner and status displays
const char* CONFIG_PORTAL_OTA_STATUS_STYLES = R"(
<style>
.spinner { border: 4px solid #f3f3f3; border-top: 4px solid #0066cc; border-radius: 50%; width: 40px; height: 40px; animation: spin 1s linear infinite; margin: 20px auto; }
@keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }
.status-box { padding: 30px; background: #e8f4f8; border-radius: 8px; text-align: center; margin: 20px 0; }
.warning-box { padding: 15px; background: #fff3cd; border-left: 4px solid #ffc107; border-radius: 4px; margin: 20px 0; }
</style>
)";

// VCOM Warning Section - Critical safety warning for VCOM management page
const char* CONFIG_PORTAL_VCOM_WARNING_HTML = R"(
<div style='background: #fef2f2; border: 2px solid #fee2e2; border-radius: 8px; padding: 15px; color: #991b1b;'>
<strong style='font-size: 16px;'>⚠️ Caution:</strong><br>
Changing the VCOM value can permanently damage your e-ink display if set incorrectly. 
Only proceed if you know what you are doing!<br><br>
<strong>Note:</strong> Programming VCOM writes to the PMIC EEPROM.
</div>
)";

// VCOM Test Pattern Section - Instructions for test pattern display
const char* CONFIG_PORTAL_VCOM_TEST_PATTERN_HTML = R"(
<div class='help-text'>
Your device is now displaying a test pattern with 8 grayscale bars. 
Compare the visual quality as you adjust VCOM values. Look for smooth gradients and good contrast.
</div>
)";

// Floating Battery Badge HTML
const char* CONFIG_PORTAL_BADGE_HTML = R"(
<div class='floating-battery-badge hidden' onclick='scrollToBattery()'>
  <div class='badge-label'>Battery Life</div>
  <div class='badge-value'>-- days</div>
  <div class='badge-icon'>👆 Click to see details</div>
</div>
)";

// Success Page Template - use with String.replace()
// %MESSAGE% - main success message
// %SUBMESSAGE% - secondary message
// %REDIRECT_INFO% - redirect information (optional)
const char* CONFIG_PORTAL_SUCCESS_PAGE_TEMPLATE = R"(
<div class='success'>
<h1>✅ Success!</h1>
<p style='margin-top: 15px;'>%MESSAGE%</p>
<p style='margin-top: 10px;'>%SUBMESSAGE%</p>
%REDIRECT_INFO%
</div>
)";

// Error Page Template - use with String.replace()
// %ERROR% - error message to display
// %REDIRECT_INFO% - redirect information (optional)
const char* CONFIG_PORTAL_ERROR_PAGE_TEMPLATE = R"(
<div class='error'>
<h1>❌ Error</h1>
<p style='margin-top: 15px;'>%ERROR%</p>
%REDIRECT_INFO%
</div>
)";

// Factory Reset Success Template
const char* CONFIG_PORTAL_FACTORY_RESET_SUCCESS = R"(
<div class='success'>
<h1>🔄 Factory Reset Complete</h1>
<p style='margin-top: 15px;'>All configuration has been erased.</p>
<p style='margin-top: 10px;'>The device will reboot now and start in setup mode.</p>
<p style='margin-top: 15px; font-size: 14px;'>Please wait for the device to restart...</p>
</div>
)";

// Reboot Success Template
const char* CONFIG_PORTAL_REBOOT_SUCCESS = R"(
<div class='success'>
<h1>🔄 Rebooting</h1>
<p style='margin-top: 15px;'>The device is restarting now.</p>
<p style='margin-top: 10px;'>Configuration has been preserved.</p>
<p style='margin-top: 15px; font-size: 14px;'>Please wait for the device to restart...</p>
</div>
)";

// OTA Status Page Content - HTML structure for firmware update progress page
const char* CONFIG_PORTAL_OTA_STATUS_CONTENT_HTML = R"(
<div class='status-box'>
<div class='spinner'></div>
<h2 id='statusTitle'>Downloading Firmware...</h2>
<p id='statusMessage'>Please wait while the firmware is being downloaded and installed.</p>
</div>

<div class='warning-box'>
<strong>⚠️ Important:</strong> Do not power off the device or close this page. The device will restart automatically when the update is complete.
</div>

<div id='progressSection' style='margin-top: 20px;'>
<div class='progress-container'>
<div class='progress-bar' id='progressBar'>0%</div>
</div>
<p style='text-align: center; margin-top: 10px; color: #666;' id='progressText'>Initializing...</p>
</div>

<div id='errorSection' style='display: none;'>
<div class='error'>
<h2>❌ Update Failed</h2>
<p id='errorMessage'></p>
<button type='button' class='btn-primary' style='margin-top: 20px;' onclick='window.location.href="/ota"'>← Back to Firmware Update</button>
</div>
</div>
)";

// Firmware Update button
const char* CONFIG_PORTAL_FIRMWARE_UPDATE_BUTTON = R"(
<div style='margin-top: 20px;'>
<a href='/ota' style='display: block; text-decoration: none;'>
<button type='button' class='btn-secondary'>⬆️ Firmware Update</button>
</a>
</div>
)";

// Reboot button
const char* CONFIG_PORTAL_REBOOT_BUTTON = R"(
<div style='margin-top: 10px;'>
<form method='POST' action='/reboot' style='display: block;'>
<button type='submit' class='btn-secondary' style='width: 100%;'>🔄 Reboot Device</button>
</form>
</div>
)";

// Factory Reset & VCOM danger zone section start
const char* CONFIG_PORTAL_DANGER_ZONE_START = R"(
<div class='factory-reset-section'>
<div class='danger-zone'>
<h2>⚠️ Danger Zone</h2>
<p>Factory reset will erase all settings including WiFi credentials and configuration. The device will reboot and start fresh.</p>
<button class='btn-danger' onclick='showResetModal()'>🗑️ Factory Reset</button>
)";

// VCOM management button (only for non-Inkplate2 boards)
const char* CONFIG_PORTAL_VCOM_BUTTON = R"(
<p style='margin-top:20px;'>VCOM management allows you to view and adjust the display panel's VCOM voltage. This is an advanced feature for correcting image artifacts or ghosting. Use with caution.</p>
<button class='btn-danger' style='width:100%; margin-top:10px;' onclick="window.location.href='/vcom'">⚠️ VCOM Management</button>
)";

// Factory Reset & VCOM danger zone section end
const char* CONFIG_PORTAL_DANGER_ZONE_END = R"(
</div>
</div>
)";

// Helper macros for generating section HTML
// Usage: SECTION_START("icon", "Title") + content + SECTION_END()
#define SECTION_START(icon, title) \
    "<div class='section'><div class='section-header'><div class='section-icon'>" icon "</div>" title "</div><div class='section-content'>"

#define SECTION_END() \
    "</div></div>"

#endif // CONFIG_PORTAL_HTML_H
