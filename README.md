# Inkplate Dashboard
A smart, multi-board firmware for [Inkplate e-ink displays by Soldered Electronics](https://www.soldered.com/categories/inkplate/) that transforms your device into a beautiful, always-on dashboard. Effortlessly download and display custom images from any web URL, with easy WiFi setup, web-based configuration, and power-saving features. Perfect for Home Assistant users, digital signage, and battery-powered displays—just set it up and enjoy a personalized, glanceable dashboard that updates itself automatically.

## Features

- 📶 **Easy WiFi setup** via captive portal
- 🖼️ **Displays PNG images** from any public URL
- 🔄 **Configurable screen rotation** (portrait/landscape)
- 🔋 **Power efficient**: deep sleep between updates
- 🌐 **Web-based configuration** and OTA firmware updates
- 🔘 **Button controls** for config mode and manual refresh
- 🏠 **Home Assistant integration** (optional MQTT battery reporting)
- ⚡ **Advanced options**: CRC32-based battery saver, flexible hourly scheduling, and more

<div align="left">
  <table>
    <tr>
      <th style="min-width:260px;">Splashscreen during first boot</th>
      <th style="min-width:220px;">Config Portal in brower</th>
    </tr>
    <tr>
      <td style="vertical-align:top; text-align:center;">
        <img src="docs/assets/device-splashscreen.jpg" alt="Inkplate Dashboard Example" style="max-width:300px; height:auto;" /><br>
      </td>
      <td style="vertical-align:top; text-align:center;">
        <img src="docs/assets/config-in-browser.gif" alt="Inkplate Dashboard - In-Browser Configuration" style="max-width:200px; height:auto;" /><br>
      </td>
    </tr>
  </table>
</div>


## Supported Devices

- **Inkplate 5 V2**
- **Inkplate 2**
- **Inkplate 6 Flick** *(not tested on hardware)*
- **Inkplate 10** *(not tested on hardware)*

> **Note:** Inkplate 10 and Inkplate 6 Flick are not tested on real hardware, as I do not own these boards. If you have one of these devices and want to help test or ensure support, please [create an issue](https://github.com/jantielens/inkplate-dashboard/issues) or open a discussion!

## Getting started
1. Go to the [GitHub Releases](https://github.com/jantielens/inkplate-dashboard/releases) page and download the latest firmware for your Inkplate model.
2. Connect your Inkplate device to your computer via USB.
3. Flash the firmware using one of these tools:
	- [ESPHome Flasher](https://github.com/esphome/esphome-flasher) (easy graphical tool)
	- [esptool.py](https://github.com/espressif/esptool) (advanced command-line utility)
	- Any compatible serial flashing utility
	Refer to your tool's documentation for instructions on uploading `.bin` files to ESP32-based boards.
4. Power on your device. The onboarding flow has two steps:
	- **Step 1: WiFi Setup (AP Mode)** – On first boot, the device creates a WiFi access point. Connect to it and enter your WiFi credentials in the browser.
	- **Step 2: Dashboard Configuration (Config Mode)** – After joining your WiFi, the device automatically enters config mode. Access the device’s IP in your browser to set your image URL and refresh rate.
	The device will guide you through each step directly on the screen.
5. Enjoy your personalized, always-on dashboard!

## Documentation

- User documentation is available in the [docs/user/README.md](docs/user/README.md) file.
- Developer documentation can be found in the [docs/dev/README.md](docs/dev/README.md) file.

## Credits

- Built with [Arduino CLI](https://arduino.github.io/arduino-cli/)
- Uses [Inkplate Library](https://github.com/SolderedElectronics/Inkplate-Arduino-library)
- For [Inkplate Devices](https://www.soldered.com/categories/inkplate/) by Soldered Electronics
