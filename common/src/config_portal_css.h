// Config Portal CSS - Mobile-optimized styles for configuration web interface
// This file contains the CSS for the configuration portal that runs in AP mode
// and config mode. The CSS is embedded in the firmware to avoid filesystem dependencies.

#ifndef CONFIG_PORTAL_CSS_H
#define CONFIG_PORTAL_CSS_H

const char* CONFIG_PORTAL_CSS = R"(
<style>
    /* Reset and base styles */
    * {
        margin: 0;
        padding: 0;
        box-sizing: border-box;
    }
    
    body {
        font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        min-height: 100vh;
        padding: 20px;
    }
    
    /* Container */
    .container {
        background: #f8f9fa;
        border-radius: 12px;
        box-shadow: 0 10px 40px rgba(0,0,0,0.2);
        padding: 40px;
        max-width: 640px;
        width: 100%;
        margin: 0 auto;
    }
    
    /* Typography */
    h1 {
        color: #333;
        margin-bottom: 10px;
        font-size: 28px;
    }
    
    .subtitle {
        color: #666;
        margin-bottom: 30px;
        font-size: 14px;
    }
    
    /* Form elements */
    .form-group {
        margin-bottom: 20px;
    }
    
    label {
        display: block;
        margin-bottom: 8px;
        color: #333;
        font-weight: 500;
        font-size: 14px;
    }
    
    input[type="text"],
    input[type="password"],
    input[type="number"],
    input[type="url"],
    select {
        width: 100%;
        padding: 12px;
        border: 2px solid #e0e0e0;
        border-radius: 6px;
        font-size: 16px;
        transition: border-color 0.3s;
        -webkit-appearance: none;
        touch-action: manipulation;
        background: white;
    }
    
    select {
        background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='12' viewBox='0 0 12 12'%3E%3Cpath fill='%23333' d='M6 9L1 4h10z'/%3E%3C/svg%3E");
        background-repeat: no-repeat;
        background-position: right 12px center;
        padding-right: 40px;
        cursor: pointer;
    }
    
    input:focus,
    select:focus {
        outline: none;
        border-color: #667eea;
    }
    
    /* Checkbox styling */
    input[type="checkbox"] {
        width: 20px;
        height: 20px;
        cursor: pointer;
        touch-action: manipulation;
    }
    
    .help-text {
        font-size: 12px;
        color: #666;
        margin-top: 5px;
    }
    
    /* Section card styling */
    .section {
        background: white;
        margin-bottom: 16px;
        border-radius: 12px;
        overflow: hidden;
        box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
    }
    
    .section-header {
        padding: 16px 20px;
        display: flex;
        align-items: center;
        gap: 12px;
        font-weight: 600;
        font-size: 16px;
        color: #212529;
        background: linear-gradient(135deg, #e3f2fd 0%, #bbdefb 100%);
        border-bottom: 3px solid #2196f3;
    }
    
    .section-icon {
        font-size: 24px;
        display: flex;
        align-items: center;
        justify-content: center;
        flex-shrink: 0;
    }
    
    .section-content {
        padding: 20px;
        background: white;
    }
    
    /* Buttons */
    button {
        width: 100%;
        padding: 14px;
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        color: white;
        border: none;
        border-radius: 6px;
        font-size: 16px;
        font-weight: 600;
        cursor: pointer;
        transition: transform 0.2s;
        touch-action: manipulation;
        -webkit-tap-highlight-color: rgba(0,0,0,0.1);
    }
    
    button:active {
        transform: translateY(0);
    }
    
    .btn-secondary {
        background: linear-gradient(135deg, #10b981 0%, #059669 100%);
        padding: 12px 24px;
        font-size: 15px;
        width: 100%;
    }
    
    /* Desktop hover effect */
    @media (hover: hover) {
        button:hover {
            transform: translateY(-2px);
        }
    }
    
    /* Status messages */
    .success {
        background: #10b981;
        color: white;
        padding: 20px;
        border-radius: 8px;
        text-align: center;
    }
    
    .error {
        background: #ef4444;
        color: white;
        padding: 20px;
        border-radius: 8px;
        text-align: center;
    }
    
    /* Device info */
    .device-info {
        background: #f3f4f6;
        padding: 15px;
        border-radius: 6px;
        margin-bottom: 20px;
        font-size: 14px;
        word-break: break-word;
    }
    
    .device-info strong {
        color: #667eea;
    }
    
    /* Factory reset section */
    .factory-reset-section {
        margin-top: 30px;
        padding-top: 30px;
        border-top: 2px solid #e0e0e0;
    }
    
    .danger-zone {
        background: #fef2f2;
        border: 2px solid #fee2e2;
        border-radius: 8px;
        padding: 20px;
    }
    
    .danger-zone h2 {
        color: #dc2626;
        font-size: 18px;
        margin-bottom: 10px;
    }
    
    .danger-zone p {
        color: #666;
        font-size: 14px;
        margin-bottom: 15px;
    }
    
    .btn-danger {
        background: #dc2626;
        color: white;
        padding: 12px 24px;
        border: none;
        border-radius: 6px;
        font-size: 14px;
        font-weight: 600;
        cursor: pointer;
        transition: background 0.3s;
        touch-action: manipulation;
    }
    
    @media (hover: hover) {
        .btn-danger:hover {
            background: #b91c1c;
        }
    }
    
    /* Modal */
    .modal {
        display: none;
        position: fixed;
        z-index: 1000;
        left: 0;
        top: 0;
        width: 100%;
        height: 100%;
        background-color: rgba(0,0,0,0.5);
        overflow-y: auto;
    }
    
    .modal-content {
        background-color: white;
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        padding: 30px;
        border-radius: 12px;
        max-width: 400px;
        width: 90%;
        box-shadow: 0 10px 40px rgba(0,0,0,0.3);
        -webkit-overflow-scrolling: touch;
    }
    
    .modal-buttons {
        display: flex;
        gap: 10px;
        margin-top: 20px;
    }
    
    .btn-cancel {
        flex: 1;
        background: #6b7280;
        color: white;
        padding: 12px;
        border: none;
        border-radius: 6px;
        font-size: 14px;
        font-weight: 600;
        cursor: pointer;
        touch-action: manipulation;
    }
    
    .btn-confirm {
        flex: 1;
        background: #dc2626;
        color: white;
        padding: 12px;
        border: none;
        border-radius: 6px;
        font-size: 14px;
        font-weight: 600;
        cursor: pointer;
        touch-action: manipulation;
    }
    
    /* OTA Update Styles */
    .warning-banner {
        background: #fef3c7;
        border: 2px solid #f59e0b;
        border-radius: 8px;
        padding: 15px;
        margin: 20px 0;
        color: #92400e;
        font-size: 14px;
        line-height: 1.6;
    }
    
    .warning-banner strong {
        color: #78350f;
    }
    
    .progress-container {
        width: 100%;
        background-color: #e5e7eb;
        border-radius: 8px;
        overflow: hidden;
        height: 40px;
        margin-top: 15px;
    }
    
    .progress-bar {
        height: 100%;
        background: linear-gradient(90deg, #667eea 0%, #764ba2 100%);
        color: white;
        display: flex;
        align-items: center;
        justify-content: center;
        font-weight: 600;
        font-size: 14px;
        transition: width 0.3s ease;
        width: 0%;
    }
    
    .btn-primary {
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        color: white;
        padding: 12px 24px;
        border: none;
        border-radius: 6px;
        font-size: 14px;
        font-weight: 600;
        cursor: pointer;
        touch-action: manipulation;
        transition: opacity 0.2s;
    }
    
    .btn-primary:hover {
        opacity: 0.9;
    }
    
    .btn-primary:disabled {
        opacity: 0.5;
        cursor: not-allowed;
    }
    
    input[type="file"] {
        padding: 10px;
        border: 2px dashed #cbd5e1;
        border-radius: 6px;
        width: 100%;
        cursor: pointer;
        background: #f9fafb;
    }
    
    input[type="file"]:hover {
        border-color: #667eea;
        background: #f3f4f6;
    }
    
    /* Battery Estimator Section */
    .battery-estimator {
        margin: 30px 0;
        padding: 20px;
        background: linear-gradient(135deg, #f5f7fa 0%, #e8ecf4 100%);
        border-radius: 10px;
        border: 2px solid #d0d8e0;
    }

    .battery-estimator-header {
        display: flex;
        align-items: center;
        gap: 10px;
        margin-bottom: 15px;
    }

    .battery-estimator-header h3 {
        color: #333;
        font-size: 18px;
        margin: 0;
    }

    .battery-capacity-input {
        margin-bottom: 15px;
    }

    .battery-capacity-input label {
        font-size: 14px;
        margin-bottom: 6px;
    }

    .battery-result {
        padding: 15px;
        background: white;
        border-radius: 8px;
        border-left: 4px solid #667eea;
        transition: border-color 0.3s;
    }

    .battery-result.excellent { 
        border-left-color: #10b981; 
    }
    
    .battery-result.good { 
        border-left-color: #3b82f6; 
    }
    
    .battery-result.moderate { 
        border-left-color: #f59e0b; 
    }
    
    .battery-result.poor { 
        border-left-color: #ef4444; 
    }

    .battery-life-display {
        font-size: 32px;
        font-weight: 700;
        color: #333;
    }

    .battery-life-subtitle {
        font-size: 14px;
        color: #666;
        margin-top: 4px;
    }

    .battery-details {
        display: grid;
        grid-template-columns: 1fr 1fr;
        gap: 10px;
        margin-top: 12px;
        padding-top: 12px;
        border-top: 1px solid #e0e0e0;
    }

    .battery-detail-item {
        display: flex;
        flex-direction: column;
        gap: 4px;
    }

    .battery-detail-label {
        font-size: 11px;
        color: #666;
        text-transform: uppercase;
        font-weight: 600;
        letter-spacing: 0.5px;
    }

    .battery-detail-value {
        font-size: 16px;
        font-weight: 600;
        color: #333;
    }

    .battery-progress {
        margin-top: 12px;
        height: 8px;
        background: #e0e0e0;
        border-radius: 4px;
        overflow: hidden;
    }

    .battery-progress-bar {
        height: 100%;
        transition: width 0.5s ease, background 0.3s ease;
    }

    .battery-progress-bar.excellent {
        background: linear-gradient(90deg, #10b981 0%, #059669 100%);
    }

    .battery-progress-bar.good {
        background: linear-gradient(90deg, #3b82f6 0%, #2563eb 100%);
    }

    .battery-progress-bar.moderate {
        background: linear-gradient(90deg, #f59e0b 0%, #d97706 100%);
    }

    .battery-progress-bar.poor {
        background: linear-gradient(90deg, #ef4444 0%, #dc2626 100%);
    }

    .status-badge {
        display: inline-block;
        padding: 4px 8px;
        border-radius: 4px;
        font-size: 11px;
        font-weight: 600;
        text-transform: uppercase;
    }

    .status-excellent { 
        background: #d1fae5; 
        color: #065f46; 
    }
    
    .status-good { 
        background: #dbeafe; 
        color: #1e40af; 
    }
    
    .status-moderate { 
        background: #fed7aa; 
        color: #92400e; 
    }
    
    .status-poor { 
        background: #fee2e2; 
        color: #991b1b; 
    }

    .battery-tips {
        margin-top: 15px;
        padding: 12px;
        background: #fef3c7;
        border-radius: 6px;
        font-size: 12px;
        color: #92400e;
        line-height: 1.5;
    }

    .battery-tips strong {
        font-weight: 700;
    }

    .assumptions {
        margin-top: 12px;
        padding: 10px;
        background: #f9fafb;
        border-radius: 6px;
        font-size: 11px;
        color: #6b7280;
        line-height: 1.6;
    }

    .assumptions-title {
        font-weight: 600;
        margin-bottom: 6px;
        color: #4b5563;
    }

    .assumptions ul {
        margin-left: 18px;
        margin-top: 6px;
    }

    .assumptions li {
        margin-bottom: 4px;
    }
    
    /* Floating Battery Badge */
    .floating-battery-badge {
        position: fixed;
        bottom: 20px;
        right: calc((100vw - 640px) / 2 + 20px);
        background: linear-gradient(135deg, #10b981 0%, #059669 100%);
        color: white;
        padding: 12px 16px;
        border-radius: 12px;
        box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
        cursor: pointer;
        transition: all 0.3s ease;
        z-index: 100;
        min-width: 100px;
        text-align: center;
    }
    
    .floating-battery-badge.hidden {
        opacity: 0;
        pointer-events: none;
        transform: translateY(20px);
    }
    
    .floating-battery-badge:hover {
        transform: translateY(-2px);
        box-shadow: 0 6px 16px rgba(0, 0, 0, 0.2);
    }
    
    .floating-battery-badge:active {
        transform: translateY(0);
    }
    
    .badge-label {
        font-size: 11px;
        text-transform: uppercase;
        letter-spacing: 0.5px;
        opacity: 0.9;
        font-weight: 600;
        margin-bottom: 4px;
    }
    
    .badge-value {
        font-size: 20px;
        font-weight: 700;
        line-height: 1.2;
    }
    
    .badge-icon {
        font-size: 10px;
        opacity: 0.8;
        margin-top: 4px;
    }
    
    /* Mobile responsive adjustments */
    @media (max-width: 600px) {
        body {
            padding: 12px;
        }
        
        .container {
            padding: 20px;
        }
        
        h1 {
            font-size: 24px;
        }
        
        .modal-content {
            width: 95%;
            padding: 20px;
        }
        
        .battery-life-display {
            font-size: 28px;
        }
        
        .floating-battery-badge {
            right: 20px;
            bottom: 20px;
        }
    }
    
    @media (max-width: 480px) {
        .section-content {
            padding: 15px;
        }
        
        .section-header {
            padding: 12px 15px;
            font-size: 15px;
        }
        
        .section-icon {
            font-size: 20px;
        }
    }
</style>
)";

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

// Battery Life Calculator JavaScript
// Calculates and displays battery life estimates based on configuration
const char* CONFIG_PORTAL_BATTERY_CALC_SCRIPT = R"(
<script>
const POWER_CONSTANTS = {
  ACTIVE_MA: 100,
  DISPLAY_MA: 50,
  SLEEP_MA: 0.02,
  IMAGE_UPDATE_SEC: 7,
  CRC32_CHECK_SEC: 1
};

function calculateBatteryLife() {
  const refreshRate = parseInt(document.getElementById('refresh').value) || 5;
  const useCRC32 = document.getElementById('crc32check').checked;
  const batteryCapacity = parseInt(document.getElementById('battery-capacity').value) || 1200;
  const dailyChanges = parseInt(document.getElementById('daily-changes').value) || 5;
  
  let activeHours = 0;
  for (let hour = 0; hour < 24; hour++) {
    const checkbox = document.getElementById('hour_' + hour);
    if (checkbox && checkbox.checked) activeHours++;
  }
  if (activeHours === 0) activeHours = 24;
  
  const wakeupsPerDay = activeHours * (60 / refreshRate);
  let dailyPower = 0;
  let activeTimeMinutes = 0;
  
  if (useCRC32) {
    const fullUpdates = Math.min(dailyChanges, wakeupsPerDay);
    const crc32Checks = wakeupsPerDay - fullUpdates;
    const activeSecondsPerUpdate = 5;
    const displaySecondsPerUpdate = 2;
    const powerPerFullUpdate = (activeSecondsPerUpdate * POWER_CONSTANTS.ACTIVE_MA / 3600) + (displaySecondsPerUpdate * POWER_CONSTANTS.DISPLAY_MA / 3600);
    dailyPower += fullUpdates * powerPerFullUpdate;
    const powerPerCRC32Check = (POWER_CONSTANTS.CRC32_CHECK_SEC * POWER_CONSTANTS.ACTIVE_MA / 3600);
    dailyPower += crc32Checks * powerPerCRC32Check;
    activeTimeMinutes = (fullUpdates * POWER_CONSTANTS.IMAGE_UPDATE_SEC + crc32Checks * POWER_CONSTANTS.CRC32_CHECK_SEC) / 60;
  } else {
    const activeSecondsPerUpdate = 5;
    const displaySecondsPerUpdate = 2;
    const powerPerFullUpdate = (activeSecondsPerUpdate * POWER_CONSTANTS.ACTIVE_MA / 3600) + (displaySecondsPerUpdate * POWER_CONSTANTS.DISPLAY_MA / 3600);
    dailyPower += wakeupsPerDay * powerPerFullUpdate;
    activeTimeMinutes = (wakeupsPerDay * POWER_CONSTANTS.IMAGE_UPDATE_SEC) / 60;
  }
  
  const sleepHours = 24 - (activeTimeMinutes / 60);
  dailyPower += sleepHours * POWER_CONSTANTS.SLEEP_MA;
  
  const batteryLifeDays = Math.round(batteryCapacity / dailyPower);
  const batteryLifeMonths = (batteryLifeDays / 30).toFixed(1);
  
  let status = 'poor', statusText = 'SHORT';
  if (batteryLifeDays >= 180) { status = 'excellent'; statusText = 'EXCELLENT'; }
  else if (batteryLifeDays >= 90) { status = 'good'; statusText = 'GOOD'; }
  else if (batteryLifeDays >= 45) { status = 'moderate'; statusText = 'MODERATE'; }
  
  document.getElementById('battery-days').textContent = batteryLifeDays + ' days';
  document.getElementById('battery-months').textContent = 'Approximately ' + batteryLifeMonths + ' months';
  document.getElementById('daily-power').textContent = dailyPower.toFixed(1) + ' mAh';
  document.getElementById('wakeups').textContent = wakeupsPerDay;
  document.getElementById('active-time').textContent = activeTimeMinutes.toFixed(1) + ' min/day';
  document.getElementById('sleep-time').textContent = sleepHours.toFixed(1) + ' hrs/day';
  
  document.getElementById('status-badge').textContent = statusText;
  document.getElementById('status-badge').className = 'status-badge status-' + status;
  document.getElementById('battery-result').className = 'battery-result ' + status;
  
  const progressBar = document.getElementById('progress-bar');
  const progressPercent = Math.min(100, (batteryLifeDays / 300) * 100);
  progressBar.style.width = progressPercent + '%';
  progressBar.className = 'battery-progress-bar ' + status;
  
  const tipsDiv = document.getElementById('battery-tips');
  const tipText = document.getElementById('tip-text');
  if (!useCRC32) {
    tipsDiv.style.display = 'block';
    tipText.textContent = 'Enable CRC32 change detection to extend battery life by 5-8×!';
  } else if (refreshRate <= 2) {
    tipsDiv.style.display = 'block';
    tipText.textContent = 'Consider increasing refresh rate to 5-10 minutes to extend battery life.';
  } else if (batteryLifeDays < 60) {
    tipsDiv.style.display = 'block';
    tipText.textContent = 'Consider using a larger battery or reducing refresh frequency.';
  } else {
    tipsDiv.style.display = 'none';
  }
}

document.addEventListener('DOMContentLoaded', function() {
  document.getElementById('refresh').addEventListener('input', calculateBatteryLife);
  document.getElementById('crc32check').addEventListener('change', calculateBatteryLife);
  document.getElementById('battery-capacity').addEventListener('change', calculateBatteryLife);
  document.getElementById('daily-changes').addEventListener('input', calculateBatteryLife);
  for (let hour = 0; hour < 24; hour++) {
    const checkbox = document.getElementById('hour_' + hour);
    if (checkbox) checkbox.addEventListener('change', calculateBatteryLife);
  }
  calculateBatteryLife();
});
</script>
)";

// Floating Battery Badge JavaScript
// Handles visibility logic and interactions for the floating badge
const char* CONFIG_PORTAL_BADGE_SCRIPT = R"(
<script>
function isBatteryDaysVisible() {
  const daysElement = document.getElementById('battery-days');
  if (!daysElement) return true;
  const rect = daysElement.getBoundingClientRect();
  return rect.top < window.innerHeight && rect.bottom > 0;
}
function isScrolledPastBatteryDays() {
  const daysElement = document.getElementById('battery-days');
  if (!daysElement) return false;
  const rect = daysElement.getBoundingClientRect();
  return rect.bottom < 0;
}
function updateBadgeVisibility() {
  const badge = document.querySelector('.floating-battery-badge');
  if (!badge) return;
  const isVisible = isBatteryDaysVisible();
  const isPast = isScrolledPastBatteryDays();
  if (!isVisible && !isPast) { badge.classList.remove('hidden'); }
  else { badge.classList.add('hidden'); }
}
function scrollToBattery() {
  const daysElement = document.getElementById('battery-days');
  if (daysElement) { daysElement.scrollIntoView({ behavior: 'smooth', block: 'center' }); }
}
function updateBadgeValue() {
  const badge = document.querySelector('.badge-value');
  const days = document.getElementById('battery-days');
  if (badge && days) { badge.textContent = days.textContent; }
}
window.addEventListener('scroll', updateBadgeVisibility);
window.addEventListener('resize', updateBadgeVisibility);
document.addEventListener('DOMContentLoaded', function() {
  updateBadgeVisibility();
  const observer = new MutationObserver(updateBadgeValue);
  const target = document.getElementById('battery-days');
  if (target) { observer.observe(target, { characterData: true, childList: true, subtree: true }); }
  updateBadgeValue();
});
</script>
)";

// Floating Battery Badge HTML
const char* CONFIG_PORTAL_BADGE_HTML = R"(
<div class='floating-battery-badge hidden' onclick='scrollToBattery()'>
  <div class='badge-label'>Battery Life</div>
  <div class='badge-value'>-- days</div>
  <div class='badge-icon'>👆 Click to see details</div>
</div>
)";

// Factory Reset Modal JavaScript
const char* CONFIG_PORTAL_MODAL_SCRIPT = R"(
<script>
function showResetModal() { document.getElementById('resetModal').style.display = 'block'; }
function hideResetModal() { document.getElementById('resetModal').style.display = 'none'; }
window.onclick = function(event) { var modal = document.getElementById('resetModal'); if (event.target == modal) { modal.style.display = 'none'; } }
</script>
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

// OTA Page - JavaScript for GitHub updates and manual upload
const char* CONFIG_PORTAL_OTA_SCRIPT = R"(
<script>
var updateAssetUrl = '';
function checkForUpdates() {
  document.getElementById('checkUpdateBtn').disabled = true;
  document.getElementById('checkLoading').style.display = 'block';
  document.getElementById('updateResults').style.display = 'none';
  document.getElementById('checkError').style.display = 'none';
  fetch('/ota/check')
    .then(function(response) { return response.json(); })
    .then(function(data) {
      document.getElementById('checkLoading').style.display = 'none';
      document.getElementById('checkUpdateBtn').disabled = false;
      if (data.success && data.found) {
        var infoHtml = '<strong>Current Version:</strong> ' + data.current_version + '<br>';
        infoHtml += '<strong>Latest Version:</strong> ' + data.latest_version + '<br>';
        infoHtml += '<strong>Asset:</strong> ' + data.asset_name + '<br>';
        infoHtml += '<strong>Size:</strong> ' + Math.round(data.asset_size / 1024) + ' KB<br>';
        if (data.is_newer) {
          infoHtml += '<div style="margin-top: 10px; padding: 8px; background: #d4edda; border-radius: 4px; color: #155724;"><strong>✓ Update Available</strong></div>';
          document.getElementById('installUpdateBtn').style.display = 'block';
          updateAssetUrl = data.asset_url;
        } else {
          infoHtml += '<div style="margin-top: 10px; padding: 8px; background: #d1ecf1; border-radius: 4px; color: #0c5460;"><strong>✓ You are up to date</strong></div>';
          document.getElementById('installUpdateBtn').style.display = 'none';
        }
        document.getElementById('updateInfo').innerHTML = infoHtml;
        document.getElementById('updateResults').style.display = 'block';
      } else {
        document.getElementById('checkErrorText').innerText = data.error || 'Unknown error';
        document.getElementById('checkError').style.display = 'block';
      }
    })
    .catch(function(error) {
      document.getElementById('checkLoading').style.display = 'none';
      document.getElementById('checkUpdateBtn').style.display = 'block';
      document.getElementById('checkErrorText').innerText = 'Network error: ' + error.message;
      document.getElementById('checkError').style.display = 'block';
    });
}
window.addEventListener('DOMContentLoaded', function() {
  checkForUpdates();
});
function installUpdate() {
  if (!updateAssetUrl) {
    alert('No update URL available');
    return;
  }
  window.location.href = '/ota/status?asset_url=' + encodeURIComponent(updateAssetUrl);
}
document.getElementById('otaForm').addEventListener('submit', function(e) {
  e.preventDefault();
  var fileInput = document.getElementById('update');
  var file = fileInput.files[0];
  if (!file) {
    alert('Please select a firmware file.');
    return;
  }
  if (!file.name.endsWith('.bin')) {
    alert('Please select a valid .bin firmware file.');
    return;
  }
  var formData = new FormData();
  formData.append('update', file);
  var xhr = new XMLHttpRequest();
  document.getElementById('uploadBtn').disabled = true;
  document.getElementById('progressSection').style.display = 'block';
  document.getElementById('statusText').innerText = 'Uploading...';
  xhr.upload.addEventListener('progress', function(e) {
    if (e.lengthComputable) {
      var percentComplete = Math.round((e.loaded / e.total) * 100);
      document.getElementById('progressBar').style.width = percentComplete + '%';
      document.getElementById('progressBar').innerText = percentComplete + '%';
    }
  });
  xhr.addEventListener('load', function() {
    if (xhr.status === 200) {
      document.getElementById('statusText').innerText = 'Upload complete! Flashing...';
      document.getElementById('otaForm').style.display = 'none';
      document.getElementById('successMessage').style.display = 'block';
      setTimeout(function() { window.location.href = '/'; }, 10000);
    } else {
      document.getElementById('errorText').innerText = 'Server returned error: ' + xhr.status;
      document.getElementById('errorMessage').style.display = 'block';
      document.getElementById('uploadBtn').disabled = false;
      document.getElementById('progressSection').style.display = 'none';
    }
  });
  xhr.addEventListener('error', function() {
    document.getElementById('errorText').innerText = 'Upload failed. Please try again.';
    document.getElementById('errorMessage').style.display = 'block';
    document.getElementById('uploadBtn').disabled = false;
    document.getElementById('progressSection').style.display = 'none';
  });
  xhr.open('POST', '/ota');
  xhr.send(formData);
});
</script>
)";

// Helper macros for generating section HTML
// Usage: SECTION_START("icon", "Title") + content + SECTION_END()
#define SECTION_START(icon, title) \
    "<div class='section'><div class='section-header'><div class='section-icon'>" icon "</div>" title "</div><div class='section-content'>"

#define SECTION_END() \
    "</div></div>"

#endif // CONFIG_PORTAL_CSS_H
