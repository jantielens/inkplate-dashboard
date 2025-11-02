/**
 * Battery Life Calculator for Inkplate Dashboard
 * Ported from config portal - estimates battery life based on configuration
 */

// =====================================================
// Power Consumption Constants
// =====================================================
// These constants match the config portal implementation

const POWER_CONSTANTS = {
  ACTIVE_MA: 100,        // WiFi active + processing
  DISPLAY_MA: 50,        // E-ink display update
  SLEEP_MA: 0.02,        // Deep sleep mode (20µA)
  IMAGE_UPDATE_SEC: 7,   // Time to download and display image
  CRC32_CHECK_SEC: 1     // Time to check if image changed
};

// =====================================================
// Battery Life Calculation
// =====================================================

/**
 * Calculate battery life based on user configuration
 * @param {Object} config - Configuration object
 * @returns {Object} Battery life estimation results
 */
function calculateBatteryLife(config) {
  const {
    batteryCapacity = 1200,     // mAh
    refreshInterval = 5,        // minutes
    activeHours = 24,           // hours per day
    dailyChanges = 5,           // expected image changes
    useCRC32 = true            // CRC32 change detection
  } = config;

  // Calculate wake-ups per day based on active hours and refresh interval
  const wakeupsPerDay = activeHours * (60 / refreshInterval);
  
  let dailyPower = 0;          // mAh per day
  let activeTimeMinutes = 0;   // Active time per day

  if (useCRC32) {
    // With CRC32: only full updates when image actually changes
    const fullUpdates = Math.min(dailyChanges, wakeupsPerDay);
    const crc32Checks = wakeupsPerDay - fullUpdates;
    
    // Power for full image updates
    const activeSecondsPerUpdate = 5;
    const displaySecondsPerUpdate = 2;
    const powerPerFullUpdate = 
      (activeSecondsPerUpdate * POWER_CONSTANTS.ACTIVE_MA / 3600) + 
      (displaySecondsPerUpdate * POWER_CONSTANTS.DISPLAY_MA / 3600);
    dailyPower += fullUpdates * powerPerFullUpdate;
    
    // Power for CRC32 checks (no display update)
    const powerPerCRC32Check = 
      (POWER_CONSTANTS.CRC32_CHECK_SEC * POWER_CONSTANTS.ACTIVE_MA / 3600);
    dailyPower += crc32Checks * powerPerCRC32Check;
    
    // Calculate total active time
    activeTimeMinutes = (
      fullUpdates * POWER_CONSTANTS.IMAGE_UPDATE_SEC + 
      crc32Checks * POWER_CONSTANTS.CRC32_CHECK_SEC
    ) / 60;
  } else {
    // Without CRC32: full update every wake-up
    const activeSecondsPerUpdate = 5;
    const displaySecondsPerUpdate = 2;
    const powerPerFullUpdate = 
      (activeSecondsPerUpdate * POWER_CONSTANTS.ACTIVE_MA / 3600) + 
      (displaySecondsPerUpdate * POWER_CONSTANTS.DISPLAY_MA / 3600);
    dailyPower += wakeupsPerDay * powerPerFullUpdate;
    
    activeTimeMinutes = (wakeupsPerDay * POWER_CONSTANTS.IMAGE_UPDATE_SEC) / 60;
  }
  
  // Add deep sleep power consumption
  const sleepHours = 24 - (activeTimeMinutes / 60);
  dailyPower += sleepHours * POWER_CONSTANTS.SLEEP_MA;
  
  // Calculate battery life
  const batteryLifeDays = Math.round(batteryCapacity / dailyPower);
  const batteryLifeMonths = (batteryLifeDays / 30).toFixed(1);
  
  // Determine status level
  let status = 'poor';
  let statusText = 'SHORT';
  if (batteryLifeDays >= 180) { 
    status = 'excellent'; 
    statusText = 'EXCELLENT'; 
  } else if (batteryLifeDays >= 90) { 
    status = 'good'; 
    statusText = 'GOOD'; 
  } else if (batteryLifeDays >= 45) { 
    status = 'moderate'; 
    statusText = 'MODERATE'; 
  }
  
  // Generate tip based on configuration
  let tip = null;
  if (!useCRC32) {
    tip = 'Enable CRC32 change detection to extend battery life by 5-8×!';
  } else if (refreshInterval <= 2) {
    tip = 'Consider increasing refresh rate to 5-10 minutes to extend battery life.';
  } else if (batteryLifeDays < 60) {
    tip = 'Consider using a larger battery or reducing refresh frequency.';
  }
  
  return {
    batteryLifeDays,
    batteryLifeMonths,
    status,
    statusText,
    dailyPower: dailyPower.toFixed(1),
    wakeupsPerDay: Math.round(wakeupsPerDay),
    activeTime: activeTimeMinutes.toFixed(1),
    sleepTime: sleepHours.toFixed(1),
    progressPercent: Math.min(100, (batteryLifeDays / 300) * 100),
    tip
  };
}

// =====================================================
// UI Update Functions
// =====================================================

/**
 * Update the UI with calculated battery life results
 * @param {Object} results - Calculation results
 */
function updateBatteryUI(results) {
  // Update main display
  const daysEl = document.getElementById('battery-days');
  const monthsEl = document.getElementById('battery-months');
  const statusBadgeEl = document.getElementById('status-badge');
  const resultEl = document.getElementById('battery-result');
  
  if (daysEl) daysEl.textContent = `${results.batteryLifeDays} days`;
  if (monthsEl) monthsEl.textContent = `Approximately ${results.batteryLifeMonths} months`;
  if (statusBadgeEl) {
    statusBadgeEl.textContent = results.statusText;
    statusBadgeEl.className = `status-badge status-${results.status}`;
  }
  if (resultEl) {
    resultEl.className = `battery-result ${results.status}`;
  }
  
  // Update metrics
  const powerEl = document.getElementById('daily-power');
  const wakeupsEl = document.getElementById('wakeups');
  const activeTimeEl = document.getElementById('active-time');
  const sleepTimeEl = document.getElementById('sleep-time');
  
  if (powerEl) powerEl.textContent = `${results.dailyPower} mAh`;
  if (wakeupsEl) wakeupsEl.textContent = results.wakeupsPerDay;
  if (activeTimeEl) activeTimeEl.textContent = `${results.activeTime} min/day`;
  if (sleepTimeEl) sleepTimeEl.textContent = `${results.sleepTime} hrs/day`;
  
  // Update progress bar
  const progressBar = document.getElementById('progress-bar');
  if (progressBar) {
    progressBar.style.width = `${results.progressPercent}%`;
    progressBar.className = `battery-progress-bar ${results.status}`;
  }
  
  // Update tips
  const tipsDiv = document.getElementById('battery-tips');
  const tipTextEl = document.getElementById('tip-text');
  if (tipsDiv && tipTextEl) {
    if (results.tip) {
      tipsDiv.style.display = 'block';
      tipTextEl.textContent = results.tip;
    } else {
      tipsDiv.style.display = 'none';
    }
  }
}

/**
 * Get current configuration from form inputs
 * @returns {Object} Current configuration
 */
function getCurrentConfig() {
  const batteryCapacity = parseInt(
    document.getElementById('battery-capacity')?.value || '1200'
  );
  const refreshInterval = parseInt(
    document.getElementById('refresh-interval')?.value || '5'
  );
  const activeHours = parseInt(
    document.getElementById('active-hours')?.value || '24'
  );
  const dailyChanges = parseInt(
    document.getElementById('daily-changes')?.value || '5'
  );
  const useCRC32 = document.getElementById('crc32-enabled')?.checked ?? true;
  
  return {
    batteryCapacity,
    refreshInterval,
    activeHours,
    dailyChanges,
    useCRC32
  };
}

/**
 * Update the displayed value for range inputs
 */
function updateRangeDisplay() {
  const refreshInterval = document.getElementById('refresh-interval');
  const refreshDisplay = document.getElementById('refresh-display');
  if (refreshInterval && refreshDisplay) {
    refreshDisplay.textContent = `${refreshInterval.value} min`;
  }
}

/**
 * Recalculate and update UI
 */
function recalculate() {
  const config = getCurrentConfig();
  const results = calculateBatteryLife(config);
  updateBatteryUI(results);
}

// =====================================================
// Initialization
// =====================================================

/**
 * Initialize battery calculator when DOM is ready
 */
function initBatteryCalculator() {
  // Perform initial calculation
  recalculate();
  
  // Setup event listeners for all inputs
  const inputs = [
    'battery-capacity',
    'refresh-interval',
    'active-hours',
    'daily-changes',
    'crc32-enabled'
  ];
  
  inputs.forEach(id => {
    const element = document.getElementById(id);
    if (element) {
      element.addEventListener('change', recalculate);
      element.addEventListener('input', () => {
        updateRangeDisplay();
        recalculate();
      });
    }
  });
  
  // Initial range display update
  updateRangeDisplay();
}

// Auto-initialize if DOM is already loaded, otherwise wait
if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', initBatteryCalculator);
} else {
  initBatteryCalculator();
}
