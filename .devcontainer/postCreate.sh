#!/usr/bin/env bash
set -euo pipefail

BOARD_URL="https://raw.githubusercontent.com/SolderedElectronics/Dasduino-Board-Definitions-for-Arduino-IDE/master/package_Dasduino_Boards_index.json"
INSTALL_SCRIPT="/tmp/install-arduino-cli.sh"

log() {
    echo "[devcontainer] $1"
}

if ! command -v arduino-cli >/dev/null 2>&1; then
    log "Installing Arduino CLI..."
    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh -o "${INSTALL_SCRIPT}"
    sudo BINDIR=/usr/local/bin sh "${INSTALL_SCRIPT}"
    rm -f "${INSTALL_SCRIPT}"
else
    log "Arduino CLI already available."
fi

CONFIG_FILE="${HOME}/.arduino15/arduino-cli.yaml"
if [ ! -f "${CONFIG_FILE}" ]; then
    log "Initializing Arduino CLI config..."
    arduino-cli config init --dest-file "${CONFIG_FILE}"
fi

log "Configuring board manager index..."
arduino-cli config set board_manager.additional_urls "${BOARD_URL}" >/dev/null

log "Updating core index..."
arduino-cli core update-index >/dev/null

if ! arduino-cli core list | grep -q "^Inkplate_Boards:esp32"; then
    log "Installing Inkplate ESP32 core..."
    arduino-cli core install Inkplate_Boards:esp32 >/dev/null
else
    log "Inkplate ESP32 core already installed."
fi

ensure_library() {
    local library="$1"
    if arduino-cli lib list --format json | python3 - "$library" <<'PY' >/dev/null 2>&1
import json
import sys

library_name = sys.argv[1]

try:
    libraries = json.load(sys.stdin)
except json.JSONDecodeError:
    raise SystemExit(1)

for entry in libraries:
    if entry.get("name") == library_name:
        raise SystemExit(0)

raise SystemExit(1)
PY
    then
        log "Arduino library ${library} already installed."
    else
        log "Installing Arduino library ${library}..."
        arduino-cli lib install "${library}" >/dev/null
    fi
}

log "Ensuring required libraries are installed..."
ensure_library "InkplateLibrary"
ensure_library "PubSubClient"

if ! python3 -c "import serial" >/dev/null 2>&1; then
    log "Installing Python pyserial dependency..."
    python3 -m pip install --user pyserial >/dev/null
else
    log "pyserial already installed."
fi

log "Development container setup complete."
