# Display Cycle Overview

This guide explains the runtime flow the Inkplate dashboard follows after a wake event, and how the different modes interact. It complements the inline comments in `common/src/main_sketch.ino.inc`.

```mermaid
flowchart TD
  Wake([Wake event]) --> P[Power manager init]
  P --> W[Detect wake reason]
  W --> C[Config manager init]
  C --> D[Read debug flag]
  D --> I[Init display]
  I --> G{Show splash?}
  G -->|Yes| H[Render splash]
  G -->|No| K[Keep previous image]
  H --> Mode
  K --> Mode

  Mode{Config state}
  Mode -->|No Wi-Fi & first boot| AP[AP setup portal]
  Mode -->|Wi-Fi only| STA[STA config portal]
  Mode -->|Long button press| BTN[Button config portal]
  Mode -->|Fully configured| NOR[Normal update]

  AP --> WaitUser[Wait for setup]
  STA --> WaitUser
  BTN --> WaitUser
  WaitUser --> Sleep[Prepare deep sleep]

  NOR --> LoadCfg[Load dashboard config]
  LoadCfg -->|Fail| CFail[Show config error]
  CFail --> Sleep5[Sleep 5 min]
  Sleep5 --> Sleep
  LoadCfg -->|Success| DebugChk{debug mode?}
  DebugChk -->|Yes| Status[Display status]
  DebugChk -->|No| NoStatus[Skip status]
  Status --> WiFi
  NoStatus --> WiFi

  WiFi[Connect Wi-Fi] -->|Fail| WiFiErr[Show Wi-Fi error]
  WiFiErr --> Sleep
  WiFi -->|Success| MQTTChk{MQTT configured?}
  MQTTChk -->|Yes| MQTT[Publish metrics]
  MQTTChk -->|No| NoMQTT[Skip MQTT]
  MQTT --> Img
  NoMQTT --> Img

  Img[Download & display image] -->|Success| ImgOk[Refresh image & publish loop time]
  ImgOk --> Sleep
  Img -->|Retries left| Retry[Increment retry counter]
  Retry --> Sleep20[Sleep 20 s]
  Sleep20 --> Wake
  Img -->|Final failure| ImgFail[Show final error & log]
  ImgFail --> Sleep

  Sleep --> Wake
```

## 1. Setup Phase

1. **Serial & power initialization** – `setup()` configures the `PowerManager` before anything else so we can interrogate the wake reason.
2. **Config manager** – Preferences storage is opened early. We read `debugMode` immediately so display rendering decisions are made before the Inkplate is awakened.
3. **Display splash policy** – The screen is only cleared and the splash shown when:
   - The wake reason is `WAKEUP_FIRST_BOOT`, or
   - `debugMode` is enabled in the stored `DashboardConfig`.
   On timer wakes with debug disabled, the previous image remains visible until the new one is ready.

## 2. Mode Selection

After setup the firmware branches according to configuration state and wake reason:

- **AP (boot) mode** – Triggered if no Wi-Fi credentials exist and the device just booted (`WAKEUP_FIRST_BOOT`). The ESP32 starts its own AP and shows onboarding instructions.
- **Auto config (Step 2)** – Wi-Fi credentials exist but no image URL. The device connects to Wi-Fi as a station, launches the portal, and guides the user through completing configuration.
- **Button-triggered config** – A long button press on wake forces config mode (with MQTT logging if configured). Short presses on button wakes initiate an immediate normal update.
- **Normal operation** – Any fully configured wake with no override runs `performNormalUpdate()`.

## 3. Normal Update Pipeline

1. **Load configuration** – Fails fast if preferences cannot be retrieved.
2. **Conditional status UI** – When `debugMode` is `true`, status messages (SSID, refresh interval, download progress, errors) are rendered to the e-ink panel. When `false`, the device stays silent until the final outcome.
3. **Wi-Fi connection** – Attempts to associate using stored credentials. On success RSSI is captured for MQTT telemetry.
4. **MQTT optional phase** – If MQTT settings are present, a publish cycle performs Home Assistant discovery, battery voltage reading, and Wi-Fi signal reporting. Failures do not block image retrieval.
5. **Download & display** – `ImageManager::downloadAndDisplay()` streams the PNG directly to the Inkplate. Success resets the retry counter and the device sleeps for the configured refresh interval.

## 4. Error and Retry Handling

- **Image retrieval failures**:
  - Up to two retries are scheduled by writing `imageRetryCount` to RTC memory and entering a 20-second deep sleep. Messages are only drawn when debug mode is on or the failure is final.
  - After the third failure a detailed error screen (debug only) is shown, MQTT logs are emitted if available, and the device sleeps until the normal refresh window.
- **Wi-Fi failures** – Without a network connection the device cannot retry immediately; it prints a diagnostic screen (debug mode) and sleeps until the refresh interval to try again.

## 5. Configuration Modes

- **Boot AP** – Displays a step-by-step guide for first-time setup, starts the portal in `BOOT_MODE`, and reboots when credentials are submitted.
- **Station portal** – Used for both auto Step 2 and manual config. Shows the IP address to browse and enforces a two-minute timeout (except when onboarding).
- **Fallback AP** – If the station portal cannot connect, the device falls back to AP mode even mid-configuration to keep the user experience consistent.

All config flows publish MQTT log entries when MQTT credentials are valid (e.g., “Config mode entered”, “Settings updated”).

## 6. Debug Mode Summary

`DashboardConfig.debugMode` affects only **on-screen** messaging:

- When `false` (default) the device minimizes e-ink refreshes: no splash on wake, no progress text, only the final image or (if unavoidable) the terminal error message.
- When `true` the previous verbose behavior is restored for troubleshooting. Serial logging is always active regardless of this flag.

To toggle the flag, use the checkbox in the configuration portal.
