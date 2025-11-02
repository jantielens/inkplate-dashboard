/**
 * Inkplate Dashboard Web Flasher - Application Logic
 * Handles device selection, manifest loading, and browser detection
 */

// =====================================================
// Configuration
// =====================================================

const BOARDS = {
  inkplate2: {
    name: 'Inkplate 2',
    icon: '📟'
  },
  inkplate5v2: {
    name: 'Inkplate 5 V2',
    icon: '📱'
  },
  inkplate6flick: {
    name: 'Inkplate 6 Flick',
    icon: '📲'
  },
  inkplate10: {
    name: 'Inkplate 10',
    icon: '🖥️'
  }
};

// =====================================================
// Initialization
// =====================================================

document.addEventListener('DOMContentLoaded', () => {
  initializeApp();
});

function initializeApp() {
  checkBrowserSupport();
  setupEventListeners();
}

// =====================================================
// Browser Support Detection
// =====================================================

function checkBrowserSupport() {
  if (!navigator.serial) {
    const warning = document.getElementById('no-support-warning');
    if (warning) {
      warning.classList.remove('hidden');
      console.warn('Web Serial API not supported');
    }
  }
}

// =====================================================
// Event Listeners Setup
// =====================================================

function setupEventListeners() {
  const installButton = document.querySelector('esp-web-install-button');
  const accordionHeaders = document.querySelectorAll('.accordion-header');

  // Generate device selection dynamically
  generateDeviceSelection(installButton);

  // Setup device image animation on scroll
  setupDeviceImageAnimation();

  // Accordion toggling
  accordionHeaders.forEach(header => {
    header.addEventListener('click', (e) => handleAccordionToggle(e));
  });
}

// =====================================================
// Device Selection Generator
// =====================================================

async function generateDeviceSelection(installButton) {
  const container = document.getElementById('device-selection');
  if (!container) return;

  for (const [boardId, boardInfo] of Object.entries(BOARDS)) {
    const label = document.createElement('label');
    label.className = 'device-option';

    const input = document.createElement('input');
    input.type = 'radio';
    input.name = 'board';
    input.value = boardId;
    input.addEventListener('change', (e) => handleBoardSelection(e, installButton));

    const deviceLabel = document.createElement('span');
    deviceLabel.className = 'device-label';

    const icon = document.createElement('span');
    icon.className = 'device-icon';
    icon.textContent = boardInfo.icon;

    const name = document.createElement('span');
    name.className = 'device-name';
    
    // Fetch version for this board
    const version = await fetchVersion(`./manifest_${boardId}.json`);
    const versionText = version ? ` - v${version}` : '';
    name.textContent = `${boardInfo.name}${versionText}`;

    deviceLabel.appendChild(icon);
    deviceLabel.appendChild(name);
    label.appendChild(input);
    label.appendChild(deviceLabel);
    container.appendChild(label);
  }
}

/**
 * Fetch version from manifest
 */
async function fetchVersion(manifestUrl) {
  try {
    const response = await fetch(manifestUrl);
    if (!response.ok) {
      return null;
    }
    const manifest = await response.json();
    return manifest.version || null;
  } catch (error) {
    console.error('Error fetching version:', error);
    return null;
  }
}

// =====================================================
// Board Selection Handler
// =====================================================

async function handleBoardSelection(event, installButton) {
  const boardId = event.target.value;
  const manifestUrl = `./manifest_${boardId}.json`;
  
  if (installButton) {
    installButton.manifest = manifestUrl;
    installButton.classList.remove('hidden');
  }
  
  console.log(`Selected board: ${boardId} (${BOARDS[boardId]?.name || 'Unknown'})`);
  console.log(`Using manifest: ${manifestUrl}`);
}

// =====================================================
// Accordion Handler
// =====================================================

function handleAccordionToggle(event) {
  const header = event.currentTarget;
  const content = header.nextElementSibling;
  
  if (!header.classList.contains('accordion-header')) {
    return;
  }

  // Close all other accordion items
  const allHeaders = document.querySelectorAll('.accordion-header');
  allHeaders.forEach(h => {
    if (h !== header) {
      h.classList.remove('active');
      const nextContent = h.nextElementSibling;
      if (nextContent && nextContent.classList.contains('accordion-content')) {
        nextContent.classList.remove('active');
      }
    }
  });

  // Toggle current accordion item
  header.classList.toggle('active');
  if (content && content.classList.contains('accordion-content')) {
    content.classList.toggle('active');
  }
}

// =====================================================
// Utility Functions
// =====================================================

/**
 * Get board information by ID
 */
function getBoardInfo(boardId) {
  return BOARDS[boardId] || null;
}

/**
 * Get all available boards
 */
function getAvailableBoards() {
  return Object.entries(BOARDS).map(([id, info]) => ({ id, ...info }));
}

// =====================================================
// Device Image Animation
// =====================================================

function setupDeviceImageAnimation() {
  const container = document.querySelector('.device-image-container');
  if (!container) return;

  let scrollStarted = false;
  let imageToggle = false;
  let animationTimeout = null;

  // Start animation on first scroll
  window.addEventListener('scroll', () => {
    if (!scrollStarted) {
      scrollStarted = true;
      startImageAnimation();
    }
  }, { once: true });

  function startImageAnimation() {
    function toggleImage() {
      imageToggle = !imageToggle;
      if (imageToggle) {
        container.classList.add('show-image-2');
      } else {
        container.classList.remove('show-image-2');
      }
      animationTimeout = setTimeout(toggleImage, 3000);
    }
    toggleImage();
  }
}

// Export for testing if needed
if (typeof module !== 'undefined' && module.exports) {
  module.exports = {
    getBoardInfo,
    getAvailableBoards,
    BOARDS
  };
}
