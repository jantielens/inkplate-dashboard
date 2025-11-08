// Config Portal JavaScript
// This file contains JavaScript code for the configuration web interface
// This code is embedded in the firmware to avoid filesystem dependencies.

#ifndef CONFIG_PORTAL_JS_H
#define CONFIG_PORTAL_JS_H

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
  // Calculate average interval from image slots
  let totalInterval = 0;
  let imageCount = 0;
  
  for (let i = 0; i < 10; i++) {
    const urlInput = document.querySelector(`input[name="img_url_${i}"]`);
    const intInput = document.querySelector(`input[name="img_int_${i}"]`);
    
    if (urlInput && urlInput.value.trim().length > 0) {
      const interval = parseInt(intInput.value) || 5;
      totalInterval += interval;
      imageCount++;
    }
  }
  
  const refreshRate = imageCount > 0 ? (totalInterval / imageCount) : 5;
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

let visibleSlots = 1;

function updateCRC32CheckboxState() {
  const crc32Checkbox = document.getElementById('crc32check');
  const crc32Warning = document.getElementById('crc32-carousel-warning');
  
  if (!crc32Checkbox) return;
  
  // Count how many image URLs are filled in visible slots
  let filledImageCount = 0;
  for (let i = 0; i < visibleSlots; i++) {
    const urlInput = document.querySelector(`input[name="img_url_${i}"]`);
    if (urlInput && urlInput.value.trim().length > 0) {
      filledImageCount++;
    }
  }
  
  // Disable CRC32 if more than 1 image is configured
  if (filledImageCount > 1) {
    crc32Checkbox.disabled = true;
    crc32Checkbox.checked = false;
    if (crc32Warning) crc32Warning.style.display = 'block';
  } else {
    crc32Checkbox.disabled = false;
    if (crc32Warning) crc32Warning.style.display = 'none';
  }
}

function checkAllHoursDisabled() {
  const warningDiv = document.getElementById('all-hours-disabled-warning');
  if (!warningDiv) return;
  
  let enabledHours = 0;
  for (let hour = 0; hour < 24; hour++) {
    const checkbox = document.getElementById('hour_' + hour);
    if (checkbox && checkbox.checked) enabledHours++;
  }
  
  if (enabledHours === 0) {
    warningDiv.style.display = 'block';
  } else {
    warningDiv.style.display = 'none';
  }
}

function updateRemoveButtons() {
  // Hide all remove buttons first
  for (let i = 1; i < 10; i++) {
    const removeBtn = document.getElementById('remove_' + i);
    if (removeBtn) {
      removeBtn.style.display = 'none';
    }
  }
  
  // Show only the remove button for the last visible slot (if > 1)
  if (visibleSlots > 1) {
    const lastVisibleIndex = visibleSlots - 1;
    const lastRemoveBtn = document.getElementById('remove_' + lastVisibleIndex);
    if (lastRemoveBtn) {
      lastRemoveBtn.style.display = 'inline-block';
    }
  }
}

function addImageSlot() {
  if (visibleSlots >= 10) return;
  
  const slot = document.getElementById('slot_' + visibleSlots);
  if (slot) {
    slot.style.display = 'block';
    visibleSlots++;
    
    // Auto-fill interval with default value
    const intInput = slot.querySelector('input[type="number"]');
    if (intInput && !intInput.value) {
      intInput.value = '5';
    }
    
    // Hide button if all 10 slots are visible
    if (visibleSlots >= 10) {
      document.getElementById('addImageBtn').style.display = 'none';
    }
    
    // Update remove button visibility
    updateRemoveButtons();
    
    // Update CRC32 checkbox state
    updateCRC32CheckboxState();
    
    // Recalculate battery life with new slot
    calculateBatteryLife();
  }
}

function removeLastImageSlot() {
  if (visibleSlots <= 1) return; // Can't remove if only 1 slot visible
  
  const lastVisibleIndex = visibleSlots - 1;
  const slot = document.getElementById('slot_' + lastVisibleIndex);
  if (!slot) return;
  
  // Clear the inputs
  const urlInput = slot.querySelector(`input[name="img_url_${lastVisibleIndex}"]`);
  const intInput = slot.querySelector(`input[name="img_int_${lastVisibleIndex}"]`);
  if (urlInput) urlInput.value = '';
  if (intInput) intInput.value = '';
  
  // Hide the slot
  slot.style.display = 'none';
  
  // Decrease visible slots count
  visibleSlots--;
  
  // Show add button again if not all slots are visible
  if (visibleSlots < 10) {
    const addBtn = document.getElementById('addImageBtn');
    if (addBtn) addBtn.style.display = 'block';
  }
  
  // Update remove button visibility
  updateRemoveButtons();
  
  // Update CRC32 checkbox state
  updateCRC32CheckboxState();
  
  // Recalculate battery life
  calculateBatteryLife();
}

// Auto-fill interval when URL is entered
function setupImageSlotListeners() {
  for (let i = 0; i < 10; i++) {
    const urlInput = document.querySelector(`input[name="img_url_${i}"]`);
    const intInput = document.querySelector(`input[name="img_int_${i}"]`);
    
    if (urlInput && intInput) {
      urlInput.addEventListener('input', function() {
        if (this.value.trim().length > 0 && !intInput.value) {
          intInput.value = '5';
        }
        updateCRC32CheckboxState();
        calculateBatteryLife();
      });
      
      intInput.addEventListener('input', calculateBatteryLife);
    }
  }
}

document.addEventListener('DOMContentLoaded', function() {
  // Initialize visible slots count based on existing data
  for (let i = 1; i < 10; i++) {
    const slot = document.getElementById('slot_' + i);
    if (slot && slot.style.display !== 'none') {
      visibleSlots = i + 1;
    }
  }
  
  // Hide add button if all slots visible
  if (visibleSlots >= 10) {
    const addBtn = document.getElementById('addImageBtn');
    if (addBtn) addBtn.style.display = 'none';
  }
  
  setupImageSlotListeners();
  
  // Update remove button visibility on initial load
  updateRemoveButtons();
  
  // Update CRC32 checkbox state on initial load
  updateCRC32CheckboxState();
  
  const crc32Check = document.getElementById('crc32check');
  if (crc32Check) crc32Check.addEventListener('change', calculateBatteryLife);
  
  const batteryCapacity = document.getElementById('battery-capacity');
  if (batteryCapacity) batteryCapacity.addEventListener('change', calculateBatteryLife);
  
  const dailyChanges = document.getElementById('daily-changes');
  if (dailyChanges) dailyChanges.addEventListener('input', calculateBatteryLife);
  
  for (let hour = 0; hour < 24; hour++) {
    const checkbox = document.getElementById('hour_' + hour);
    if (checkbox) {
      checkbox.addEventListener('change', function() {
        calculateBatteryLife();
        checkAllHoursDisabled();
      });
    }
  }
  
  // Initial check for all hours disabled warning
  checkAllHoursDisabled();
  
  calculateBatteryLife();
});

// Toggle Static IP fields visibility
function toggleStaticIPFields() {
  const staticMode = document.getElementById('ip_mode_static');
  const staticFields = document.getElementById('static_ip_fields');
  
  if (staticMode && staticFields) {
    if (staticMode.checked) {
      staticFields.style.display = 'block';
      // Make fields required when static mode is selected
      document.getElementById('static_ip').required = true;
      document.getElementById('gateway').required = true;
      document.getElementById('subnet').required = true;
      document.getElementById('dns1').required = true;
    } else {
      staticFields.style.display = 'none';
      // Remove required when DHCP mode is selected
      document.getElementById('static_ip').required = false;
      document.getElementById('gateway').required = false;
      document.getElementById('subnet').required = false;
      document.getElementById('dns1').required = false;
    }
  }
}

// Validate IPv4 format
function validateIPv4(input) {
  const ipPattern = /^(\d{1,3}\.){3}\d{1,3}$/;
  if (!ipPattern.test(input.value)) {
    return false;
  }
  
  // Check each octet is 0-255
  const octets = input.value.split('.');
  for (let i = 0; i < 4; i++) {
    const octet = parseInt(octets[i]);
    if (octet < 0 || octet > 255) {
      return false;
    }
  }
  
  return true;
}

// Add validation listeners for static IP fields
document.addEventListener('DOMContentLoaded', function() {
  const staticIPFields = ['static_ip', 'gateway', 'subnet', 'dns1', 'dns2'];
  staticIPFields.forEach(function(fieldId) {
    const field = document.getElementById(fieldId);
    if (field) {
      field.addEventListener('input', function() {
        if (this.value.length > 0 && !validateIPv4(this)) {
          this.setCustomValidity('Invalid IPv4 address (format: xxx.xxx.xxx.xxx, each part 0-255)');
        } else {
          this.setCustomValidity('');
        }
      });
    }
  });
  
  // Initialize static IP fields visibility on page load
  toggleStaticIPFields();
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

// Factory Reset Modal JavaScript
const char* CONFIG_PORTAL_MODAL_SCRIPT = R"(
<script>
function showResetModal() { document.getElementById('resetModal').style.display = 'block'; }
function hideResetModal() { document.getElementById('resetModal').style.display = 'none'; }
window.onclick = function(event) { var modal = document.getElementById('resetModal'); if (event.target == modal) { modal.style.display = 'none'; } }
</script>
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

// OTA Status Page - JavaScript for progress polling and update management
// Handles downloading, progress tracking, error handling, and automatic reboot detection
const char* CONFIG_PORTAL_OTA_STATUS_SCRIPT = R"(
<script>
var updateStarted = false;
var progressInterval = null;
var failedPolls = 0;
function updateProgress() {
  fetch('/ota/progress')
    .then(function(response) { return response.json(); })
    .then(function(data) {
      failedPolls = 0;
      if (data.inProgress) {
        var percent = data.percentComplete;
        var kb = Math.round(data.bytesDownloaded / 1024);
        var totalKb = Math.round(data.totalBytes / 1024);
        document.getElementById('progressBar').style.width = percent + '%';
        document.getElementById('progressBar').innerText = percent + '%';
        if (totalKb > 0) {
          document.getElementById('progressText').innerText = kb + ' KB / ' + totalKb + ' KB';
        }
      } else if (data.percentComplete === 100) {
        clearInterval(progressInterval);
        document.getElementById('statusTitle').innerText = 'Installing...';
        document.getElementById('statusMessage').innerText = 'Firmware downloaded. Installing and rebooting...';
        document.getElementById('progressBar').style.width = '100%';
        document.getElementById('progressBar').innerText = '100%';
      }
    })
    .catch(function(error) {
      failedPolls++;
      if (failedPolls >= 3) {
        clearInterval(progressInterval);
        document.querySelector('.spinner').style.display = 'none';
        document.getElementById('statusTitle').innerText = 'Device is Rebooting';
        document.getElementById('statusMessage').innerText = 'The firmware has been installed successfully. The device is now rebooting...';
        document.getElementById('progressBar').style.width = '100%';
        document.getElementById('progressBar').innerText = '100%';
        document.getElementById('progressText').innerText = 'Complete';
      }
    });
}
window.addEventListener('DOMContentLoaded', function() {
  var urlParams = new URLSearchParams(window.location.search);
  var assetUrl = urlParams.get('asset_url');
  if (!assetUrl) {
    document.getElementById('statusTitle').innerText = 'Error';
    document.getElementById('statusMessage').innerText = 'Missing update URL';
    document.querySelector('.spinner').style.display = 'none';
    return;
  }
  if (updateStarted) return;
  updateStarted = true;
  progressInterval = setInterval(updateProgress, 500);
  var formData = new FormData();
  formData.append('asset_url', assetUrl);
  fetch('/ota/install', { method: 'POST', body: formData })
    .then(function(response) { return response.json(); })
    .then(function(data) {
      if (data.success) {
        document.getElementById('statusTitle').innerText = 'Downloading...';
        document.getElementById('statusMessage').innerText = 'Firmware is being downloaded and installed. The device will reboot automatically when complete.';
      } else {
        clearInterval(progressInterval);
        document.querySelector('.status-box').style.display = 'none';
        document.getElementById('errorMessage').innerText = data.error || 'Unknown error';
        document.getElementById('errorSection').style.display = 'block';
      }
    })
    .catch(function(error) {
      clearInterval(progressInterval);
      document.querySelector('.status-box').style.display = 'none';
      document.getElementById('errorMessage').innerText = 'Network error: ' + error.message;
      document.getElementById('errorSection').style.display = 'block';
    });
});
</script>
)";

// Friendly Name Sanitization Preview JavaScript
const char* CONFIG_PORTAL_FRIENDLY_NAME_SCRIPT = R"(
<script>
function sanitizeFriendlyName(input) {
  let output = '';
  
  // Convert to lowercase and filter valid characters
  for (let i = 0; i < input.length && output.length < 24; i++) {
    let c = input.charAt(i);
    
    // Convert uppercase to lowercase
    if (c >= 'A' && c <= 'Z') {
      c = c.toLowerCase();
    }
    
    // Only allow lowercase letters, digits, and hyphens
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c === '-') {
      output += c;
    }
  }
  
  // Remove leading hyphens
  while (output.length > 0 && output.charAt(0) === '-') {
    output = output.substring(1);
  }
  
  // Remove trailing hyphens
  while (output.length > 0 && output.charAt(output.length - 1) === '-') {
    output = output.substring(0, output.length - 1);
  }
  
  return output;
}

function sanitizeFriendlyNamePreview() {
  const input = document.getElementById('friendlyname');
  const preview = document.getElementById('friendlyname-preview');
  
  if (!input || !preview) return;
  
  const inputValue = input.value;
  const sanitized = sanitizeFriendlyName(inputValue);
  
  if (inputValue.length === 0) {
    preview.innerHTML = '';
  } else if (sanitized.length === 0) {
    preview.innerHTML = '<span style="color: #d32f2f;">❌ Invalid: No valid characters remaining</span>';
  } else if (inputValue !== sanitized) {
    preview.innerHTML = '<span style="color: #1976d2;">✓ Your input will be used as device name. Internally used as <code style="background: #f5f5f5; padding: 2px 6px; border-radius: 3px; font-family: monospace;">' + sanitized + '</code> for MQTT topics, entity IDs, and hostname.</span>';
  } else {
    preview.innerHTML = '<span style="color: #388e3c;">✓ Valid device name (will be used for MQTT topics, entity IDs, and hostname)</span>';
  }
}

// Initialize preview on page load
document.addEventListener('DOMContentLoaded', function() {
  sanitizeFriendlyNamePreview();
});
</script>
)";

#endif // CONFIG_PORTAL_JS_H
