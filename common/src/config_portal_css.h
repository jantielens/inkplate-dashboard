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
        background: white;
        border-radius: 12px;
        box-shadow: 0 10px 40px rgba(0,0,0,0.2);
        padding: 40px;
        max-width: 500px;
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
    input[type="url"] {
        width: 100%;
        padding: 12px;
        border: 2px solid #e0e0e0;
        border-radius: 6px;
        font-size: 16px;
        transition: border-color 0.3s;
        -webkit-appearance: none;
        touch-action: manipulation;
    }
    
    input:focus {
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

#endif // CONFIG_PORTAL_CSS_H
