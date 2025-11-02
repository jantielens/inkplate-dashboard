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
  detectRunningMode();
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
// Running Mode Detection (Local vs Production)
// =====================================================

function detectRunningMode() {
  const hostname = window.location.hostname;
  const isLocal = 
    hostname === 'localhost' || 
    hostname === '127.0.0.1' ||
    hostname.includes('github.dev');
  
  if (isLocal) {
    const warning = document.getElementById('local-test-warning');
    if (warning) {
      warning.classList.remove('hidden');
    }
    console.log('🧪 Running in local testing mode - using local binaries');
  }
  
  return isLocal;
}

// =====================================================
// Event Listeners Setup
// =====================================================

function setupEventListeners() {
  const installButton = document.querySelector('esp-web-install-button');
  const boardRadios = document.querySelectorAll('input[name="board"]');
  const accordionHeaders = document.querySelectorAll('.accordion-header');

  // Board selection
  boardRadios.forEach(radio => {
    radio.addEventListener('change', (e) => handleBoardSelection(e, installButton));
  });

  // Install button state changes
  if (installButton) {
    installButton.addEventListener('state-changed', (e) => {
      console.log('Install button state changed:', e.detail);
    });
  }

  // Accordion toggling
  accordionHeaders.forEach(header => {
    header.addEventListener('click', (e) => handleAccordionToggle(e));
  });
}

// =====================================================
// Board Selection Handler
// =====================================================

function handleBoardSelection(event, installButton) {
  const boardId = event.target.value;
  const isLocal = window.location.hostname === 'localhost' || 
                  window.location.hostname === '127.0.0.1' ||
                  window.location.hostname.includes('github.dev');
  
  // Build manifest URL with optional local suffix
  const manifestSuffix = isLocal ? '_local' : '';
  const manifestUrl = `./manifest_${boardId}${manifestSuffix}.json`;
  
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

// Export for testing if needed
if (typeof module !== 'undefined' && module.exports) {
  module.exports = {
    getBoardInfo,
    getAvailableBoards,
    BOARDS
  };
}
