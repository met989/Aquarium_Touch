# Aquarium OS Touch 🐠

[🇬🇧 English](README_EN.md) | [🇮🇹 Italiano](README.md)

Advanced firmware for **ESP32** microcontrollers dedicated to complete aquarium monitoring, control, and automation via a color touchscreen display and integrated web server.

---

## 📋 Table of Contents
1. [Project Overview](#-project-overview)
2. [Key Features](#-key-features)
3. [Hardware Specifications & Pinout](#-hardware-specifications--pinout)
4. [User Interface & Touch](#-user-interface--touch)
5. [Web Server & REST APIs](#-web-server--rest-apis)
6. [Boot Animation (Boot GIF)](#-boot-animation-boot-gif)
7. [Localization & Languages](#-localization--languages)
8. [Software Architecture & File Tree](#-software-architecture--file-tree)
9. [Configuration & Persistence](#-configuration--persistence)
10. [Firmware Updates](#-firmware-updates-update)
11. [Compilation & Development](#-compilation--development-for-developers)
12. [Future Developments (Roadmap)](#-future-developments-roadmap)
13. [Support the Project](#-support-the-project)

---

## 🚀 Project Overview

**Aquarium OS Touch** is an aquarium management system built on the Arduino framework for ESP32. It combines a modern graphical user interface (*Glassmorphism* style with real-time animations) on a **2.8" (320x240) ILI9341 TFT display**. The firmware was designed and optimized natively for the **ESP32-2432S028** all-in-one board (commonly known as *Cheap Yellow Display* or *CYD*), providing a powerful backend with NTP synchronization, lighting relay management, temperature monitoring via the waterproof **DS18B20** sensor, and an **HTTP Web Server** for remote control from a browser or home automation systems like Home Assistant.

### 🖼️ Interface Previews

<div align="center">

**Touch Display:**<br>
<img src="scheme/1.jpg?v=v0.6" alt="Dashboard" width="400"/>
<img src="scheme/2.jpg?v=v0.6" alt="Light" width="400"/>
<img src="scheme/3.jpg?v=v0.6" alt="Schedule" width="400"/>
<img src="scheme/4.jpg?v=v0.6" alt="Settings" width="400"/>
<img src="scheme/5.jpg?v=v0.6" alt="Settings 2" width="400"/>
<img src="scheme/6.jpg?v=v0.6" alt="Photo" width="400"/>

<br><br>

**Web Interface:**<br>
<img src="scheme/1.png?v=v0.6" alt="Web Dashboard" />
<img src="scheme/2.png?v=v0.6" alt="Web Settings" />

<br><br>

**Demo Video:**<br>
<img src="scheme/video1.gif?v=v0.6" alt="Video Demo" />

</div>

---

## ✨ Key Features

- **Touchscreen Display & Advanced Graphics:**
  - 320x240 pixel resolution (RGB565) on ILI9341 driver.
  - XPT2046 resistive touchscreen with calibration support and a touch QWERTY keyboard with three layouts (lowercase, uppercase, numbers & symbols).
  - Responsive interface with 4 main tabs: *Dashboard*, *Light*, *Schedule*, *Settings*.
  - Animated marine theme background with dynamic waves, floating bubbles, and swimming fish.
  - Circular animated needle temperature indicator.
  - Screensaver and power-saving mode with timed backlight shutoff.
  - **Ambient Light Sensor & Auto-Dimming:** Automatic display brightness adjustment (TFT_BL) according to room lighting using the onboard GT36516 photoresistor. Configurable from both the touchscreen and web interface with real-time reading (mV).
- **Temperature Control:**
  - Continuous reading from a DS18B20 sensor over 1-Wire bus.
  - Configurable optimal temperature thresholds (default min/max: 24.0°C – 27.0°C) with 0.3°C hysteresis and offset calibration.
  - Visual status alerts: *Optimal* (Emerald Green), *Too Cold* (Blue), *Too Warm* (Coral Red).
- **Lighting Management & Automation:**
  - Relay output control for aquarium lamp or equipment (PIN 27).
  - Safe hardware initialization (Anti-Glitch) to prevent accidental relay pulses on board boot or reboot.
  - Configurable relay polarity: *Active HIGH* or *Active LOW* (Inverted relay).
  - Automated timed schedule mode (e.g. Turn ON at 10:00, Turn OFF at 18:00).
  - Quick manual override from touchscreen or web interface.
- **Wi-Fi Connectivity & Network Services:**
  - Asynchronous Wi-Fi scanning directly from the touch interface with on-screen network selection and password entry.
  - Time synchronization via NTP servers (`pool.ntp.org`, `time.nist.gov`) with timezone management and **automatic European Daylight Saving Time (EU DST - CET/CEST)** switching, toggleable via touchscreen (`DST: AUTO`) or Web UI.
  - Static IP or DHCP support.
  - **Safety & Cold Boot Anti-Crash:** Optimized Wi-Fi radio initialization to prevent crashes or kernel panics during startup. If connection attempts fail 3 times, auto-retries pause until manually re-triggered.
  - **Dynamic & Advanced MQTT Integration:** Broker configuration updated in real-time "on the fly" (every 30s) from the Web UI without needing to reboot the ESP32. Includes Home Assistant auto-discovery.
  - **Minimalist Status Indicators:** Clean status badges (Green = Connected, Yellow = Connecting/Waiting, Red = Error) showing Wi-Fi and MQTT state at a glance.
- **Integrated Web Server & REST APIs:**
  - Responsive Web Dashboard to monitor temperatures, control lights, configure schedules, and adjust settings.
  - JSON HTTP REST API for integration with Home Assistant, Node-RED, or custom scripts.
  - Remote management of the raw configuration file (`config.cfg`).
- **Multilingual System:**
  - Dynamic loading of `.lng` language files from SD card (`/languages/`) or embedded within internal Flash memory.
  - Automatic header generation (`embedded_languages.h`) at build time via Python/SCons script (`embed_languages.py`).
  - Languages included: Italian (`italian.lng`), English (`english.lng`).
- **SD Card Persistence:**
  - All user settings are saved in `/config.cfg` using temporary files (`.tmp`) and backup copies (`.bak`) to prevent corruption.

---

## 🛠 Hardware Specifications & Pinout

The project is optimized for the **ESP32-2432S028 (2.8" All-in-One)** board ([available on AliExpress](https://it.aliexpress.com/item/1005005262421075.html)).

<img src="scheme/scheme1.png?v=v0.6" alt="board" />

> [!NOTE]
> The [macsbug page (ESP32-2432S028)](https://macsbug.wordpress.com/2022/08/17/esp32-2432s028/) details the complete hardware. There are 3 hardware revisions of this board: **the latest version** (recommended) features both **USB-C and Micro USB** ports simultaneously.

All pin and hardware parameter definitions are centralized in `include/config.h` (**Single Source of Truth**).

| Component | Peripheral | ESP32 Pin | Notes / Description |
| :--- | :--- | :--- | :--- |
| **TFT Display (ILI9341)** |
| | `TFT_MOSI` | `GPIO 13` | Display SPI MOSI |
| | `TFT_MISO` | `GPIO 12` | Display SPI MISO |
| | `TFT_SCLK` | `GPIO 14` | Display SPI Clock |
| | `TFT_CS` | `GPIO 15` | Display Chip Select |
| | `TFT_DC` | `GPIO 2` | Data / Command Select |
| | `TFT_RST` | `-1` | Hardware Reset (tied to EN/VCC) |
| | `TFT_BL` | `GPIO 21` | Backlight Control (PWM / High-Low) |
| **Touchscreen (XPT2046)** |
| | `TOUCH_CS` | `GPIO 33` | Touch Chip Select |
| | `TOUCH_IRQ` | `GPIO 36` | Touch Interrupt (Input) |
| | `TOUCH_MOSI` | `GPIO 32` | Touch SPI MOSI |
| | `TOUCH_MISO` | `GPIO 39` | Touch SPI MISO |
| | `TOUCH_SCK` | `GPIO 25` | Touch SPI Clock |
| **SD Card** | `SD_CS` | `GPIO 5` | SD Card Chip Select |
| **Temperature Sensor** | `TEMP_SENSOR_PIN` | `GPIO 22` | DS18B20 1-Wire Bus (on CN1, onboard pull-up or 4.7kΩ required) |
| **Light Relay** | `LIGHT_RELAY_PIN` | `GPIO 27` | Relay output for lighting control (on CN1) |
| **pH Sensor** | `PH_SENSOR_PIN` | `GPIO 35` | Analog input for pH probe reading (on P3) |
| **I2C Bus (SDA)** | `I2C_SDA_PIN` | `GPIO 1` | SDA pin for additional I2C devices (P1 TX) |
| **I2C Bus (SCL)** | `I2C_SCL_PIN` | `GPIO 3` | SCL pin for additional I2C devices (P1 RX) |
| **Status LED (Red)** | `LED_RED_PIN` | `GPIO 4` | Indicates system error or alarm |
| **Status LED (Green)**| `LED_GREEN_PIN` | `GPIO 16` | Indicates system ready and healthy |
| **Status LED (Blue)** | `LED_BLUE_PIN` | `GPIO 17` | Indicates boot / loading in progress |
| **Light Sensor (LDR)**| `LDR_PIN` | `GPIO 34` | Onboard GT36516 photoresistor (Analog Input) |
| **Boot Button** | `BOOT_BTN` | `GPIO 0` | Onboard BOOT button |

### SPI Bus Specifications
- **Display SPI Frequency:** 27 MHz
- **Display Read SPI Frequency:** 20 MHz
- **Touch SPI Frequency:** 2.5 MHz
- **SPI Port:** HSPI

---

## 🎨 User Interface & Touch

The user interface (UI) is developed with the `TFT_eSPI` library using a full-width **TFT_eSprite** buffer (320x172 px) to completely eliminate screen flickering.

> [!TIP]
> **Recovery Mode (Touch Recalibration):** If touch input becomes inaccurate or misaligned, recalibration is quick and easy: press the **Reset** button once and hold down the **Boot** button for **3 seconds**. The system will enter Recovery Mode and guide you through an on-screen crosshair calibration.

### Navigation Tabs
1. **Dashboard:** Displays the analog needle temperature gauge, probe status, recorded min/max values, current date/time, and light status with an active marine animation.
2. **Light:** Allows manual toggling of the light relay, enabling/disabling automated schedule mode, and inverting relay logic (Active HIGH / LOW).
3. **Schedule:** Configuration of light turn ON and turn OFF times.
4. **Settings:** Multi-page menu (4 items per page) for configuring:
   - **1. Date & Time:** Date, time, timezone, and NTP sync settings.
   - **2. Target Temp:** Minimum/maximum temperature thresholds and calibration offset.
   - **3. Target pH:** Alarm thresholds for pH limits and probe offset.
   - **4. Light Polarity:** Relay logic setting (Active HIGH / Active LOW).
   - **5. Wi-Fi:** Network connection, on-screen keyboard scanning, and IP management.
   - **6. System Info:** Firmware version, free memory, and framework details.
   - **7. Language:** System language selection (e.g. Italian, English).
   - **8. Power Saving:** Screensaver timeout and display backlight shutoff.
   - **9. Factory Reset:** Reset all parameters to factory defaults.
   - **10. Hardware & I2C:** On-demand I2C hardware bus scanner.
   - **11. Screen:** Manual brightness adjustment or Auto-Dimming (with live LDR reading).
   - **12. Light Timer:** Quick access to the light schedule timer.

---

## 🌐 Web Server & REST APIs

When connected to Wi-Fi, the embedded Web Server responds on port **80**, serving a control dashboard and JSON endpoints.

### Main API Endpoints
- `GET /` : Main control web dashboard (responsive).
- `GET /api/status` : Returns current system telemetry (temperature, relay, date, time, Wi-Fi, memory).
- `POST /api/light/toggle` : Toggles the manual light state.
- `POST /api/auto/toggle` : Enables/disables automated scheduling.
- `POST /api/schedule` : Sets turn-on and turn-off schedule times.
- `POST /api/temp/settings` : Configures min/max target temperatures and offset.
- `POST /api/relay/invert` : Inverts relay switching logic.
- `POST /api/wifi/settings` : Sets Wi-Fi credentials and IP configuration.
- `POST /api/mqtt/settings` : Configures the MQTT broker settings.
- `POST /api/settings/screen` : Sets display brightness and auto-dimming options.
- `POST /api/screensaver` : Sets screensaver timeout in seconds.
- `POST /api/language` : Changes active system language.
- `GET /api/config/raw` : Downloads the raw `config.cfg` file.
- `POST /api/config/raw` : Overwrites `config.cfg` with new content.

---

## 🎬 Boot Animation (Boot GIF)

The system supports custom GIF animations played on the screen during startup.

### How to Use:
1. Prepare an animated GIF file and name it **exactly** `boot.gif` (lowercase).
2. Save `boot.gif` in the **root directory of the SD card**.
3. Insert the SD card into the ESP32 and power on. The animation will play during the first 5 seconds of boot.

### Recommended GIF Specifications:
- **Maximum Resolution:** `320x240` pixels (smaller files are centered on a black background).
- **File Size:** Preferably under **1 MB** to guarantee smooth SD card streaming.
- **Recommended Framerate:** **10–15 FPS**.
- **Transparency:** Supported.

*Note: If `boot.gif` is not found or the SD card is missing, the system will display the default textual boot screen.*

---

## 🌐 Localization & Languages

Dynamic localization is handled using `.lng` text files stored in the `/languages/` directory of the SD card or compiled directly into internal Flash memory.

### Automatic Embedded Language Generation
When building with PlatformIO, the `embed_languages.py` script reads all `.lng` files inside `languages/` and automatically generates `include/embedded_languages.h`. This ensures the system runs in the selected language even without an SD card.

---

## 📁 Software Architecture & File Tree

```
Aquarium_Touch/
├── .agents/
│   └── AGENTS.md               # AI coding agent rules and workflows
├── homeassistant/
│   ├── aquarium_dashboard.yaml # Complete Lovelace dashboard YAML
│   ├── aquarium_view.yaml      # Multi-column unbundled view YAML
│   ├── aquarium_single_card.yaml # Single-card manual editor YAML
│   ├── mqtt_entities.yaml      # MQTT switch & button configuration
│   └── README.md               # Home Assistant integration guide
├── include/
│   ├── config.h                # Single Source of Truth (Pinout, RGB565 Colors, Defaults)
│   ├── aquarium_logic.h        # AquariumLogic class (Temp, Relay, Wi-Fi, SD)
│   ├── aquarium_ui.h           # AquariumUI class (TFT_eSPI rendering, Animations, Touch)
│   ├── aquarium_server.h       # HTTP Web Server & REST API definition
│   ├── language_manager.h      # i18n localization dictionary manager
│   ├── embedded_languages.h    # Auto-generated header with languages in Flash
│   ├── embedded_web.h          # Auto-generated header with GZIP compressed Web UI
├── src/
│   ├── main.cpp                # Setup, Main loop, Boot GIF handler, Touch calibration
│   ├── aquarium_logic.cpp      # Business logic, schedule, DS18B20 sensor, SD manager
│   ├── aquarium_ui.cpp         # TFT graphical UI, menus, touch keyboard, animations
│   ├── aquarium_server.cpp     # Web Server implementation & JSON API endpoints
│   ├── aquarium_mqtt.cpp       # MQTT client & Home Assistant auto-discovery
│   └── language_manager.cpp    # Language loading and parsing of .lng files
├── languages/
│   ├── italian.lng             # Italian localization file
│   └── english.lng             # English localization file
├── scripts/
│   ├── embed_languages.py      # Pre-build SCons script for automatic language embedding
│   ├── extract_html.py         # Pre-build SCons script for HTML minification & GZIP
│   └── copy_release.py         # Post-build script copying binary to release folder
├── web/
│   └── index.html              # Web UI source (HTML/CSS/JS Dashboard)
├── boot.gif                    # Example boot animation for SD card
├── build.bat                   # Offline local build script
├── flash.bat                   # Smart offline USB firmware flasher
├── publish_release.bat         # Automated GitHub Actions release publisher
└── platformio.ini              # PlatformIO environment config and dependencies
```

---

## ⚙️ Configuration & Persistence

System settings follow a strict hierarchy:
1. **Default Configurations (`include/config.h`):** Contains compiled default values for hardware, pinout, temperature thresholds, and colors.
2. **Runtime Configuration (`/config.cfg` on SD card):** Saves and updates user changes in real-time (Wi-Fi, schedule times, target temperatures, language, timezone).
3. **Safety Fail-Safe Mechanism:** Every write operation writes first to `/config.tmp` and maintains a `/config.bak` copy to prevent corruption if power is lost.

---

## 🔄 Firmware Updates (Update)

Keeping Aquarium OS Touch updated is straightforward and requires no programming knowledge:

### Method 1: OTA (Over-The-Air) Web Update
If the device is connected to Wi-Fi, update wirelessly from any browser:
1. **Download the Update:** Visit the [Releases](https://github.com/met989/Aquarium_Touch/releases) page and download the latest `.bin` file (e.g. `esp32_2432S028_v3_0.6.bin`).
2. **Open the Web UI:** Navigate to your aquarium's IP address in a browser (e.g. `http://192.168.1.100`).
3. **Launch OTA:** Click the gold **"🔄 OTA Update"** button at the top.
4. **Upload Firmware:** The secure ElegantOTA page will open. Select the downloaded `.bin` file and wait for the upload to complete. The device will reboot automatically with the new firmware!

### Method 2: Offline USB Flasher
If Wi-Fi is not configured or you prefer a traditional cable update:
1. **Download the Update:** Download the latest `.bin` file from [Releases](https://github.com/met989/Aquarium_Touch/releases).
2. **Place the File:** Put it inside the `release/` folder of your local project copy.
3. **Connect the Display:** Plug the ESP32 board into your computer with a USB cable.
4. **Run the Flasher:** Double-click **`flash.bat`** (Smart Offline Flasher). The script will:
   - Automatically detect the USB COM port without requiring extra drivers.
   - Let you select the firmware file to install.
   - Flash the operating system in seconds.

> [!TIP]
> **Operations Log (`operations.log`):** All `.bat` executables log their execution history to `operations.log`, so you always know when your last update was performed!

---

## 🛠 Compilation & Development (For Developers)

### Requirements
- **VS Code** with **PlatformIO IDE** extension (or PlatformIO CLI).
- ESP32 Dev Module board connected via USB.

### Library Dependencies (`platformio.ini`)
- `bodmer/TFT_eSPI @ ^2.5.43`
- `XPT2046_Touchscreen`
- `OneWire`
- `DallasTemperature` (Arduino-Temperature-Control-Library)
- `bitbank2/AnimatedGIF @ ^1.4.7`

> [!TIP]
> **Recommended Graphics Library:** While the project currently uses `TFT_eSPI`, the most performant and recommended library for the **ESP32-2432S028** hardware is **LovyanGFX**. In future firmware revisions, the rendering engine may migrate to LovyanGFX to take full advantage of hardware capabilities.

### PlatformIO Commands
- **Compile firmware:**
  ```bash
  pio run
  ```
- **Flash firmware to ESP32 (Upload):**
  ```bash
  pio run --target upload
  ```
- **Serial Monitor (115200 baud):**
  ```bash
  pio run --target monitor
  ```

---

## 🔮 Future Developments (Roadmap)

The project is actively developed. Upcoming features and hardware integrations include:

- **pH Sensor:** Real-time water acidity monitoring.
- **Water Level Sensor:** Evaporation and critical level alerts on-screen and via web.
- **Automatic Feeder Actuator:** Schedule integration for timed feedings.
- **Gravity: Analog TDS Sensor Meter (Arduino):** Total Dissolved Solids (TDS) measurement in *ppm*, ideal for tracking fertilizer concentrations and overall water purity.

---

## ☕ Support the Project

If this firmware has been helpful for your aquarium or you appreciate open-source projects, consider supporting development by buying me a virtual coffee! ❤️

[![Buy me a coffee on PayPal](https://img.shields.io/badge/Buy_me_a_coffee-PayPal-00457C.svg?style=for-the-badge&logo=paypal)](https://paypal.me/MatteoSacanna)

---
