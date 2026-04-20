# Quick Start: ESP32-S3 Companion Computer for ArduPilot

**Branch**: `001-esp32-companion-computer`

This guide gets a developer from a clean checkout to a running device in one
session, then explains how to deploy all subsequent changes wirelessly via OTA.

---

## Prerequisites

| Tool | Version | Install |
|---|---|---|
| VS Code | ≥ 1.87 | https://code.visualstudio.com |
| PlatformIO IDE extension | ≥ 6.x | VS Code Extensions panel |
| Python | 3.11+ | https://www.python.org |
| USB-C cable | — | Connected to ESP32-S3-CAM USB/UART port |

You do **not** need the ESP-IDF toolchain directly — PlatformIO manages it.

---

## 1. Clone and Open

```powershell
git clone <repo-url> ardupilot-webserver
cd ardupilot-webserver
code .
```

PlatformIO will detect `platformio.ini` and download the ESP32 toolchain and
libraries on first open (~3–5 minutes).

---

## 2. Build

```powershell
# From the PlatformIO toolbar: Build  (or)
pio run -e esp32s3-cam
```

Expected output (last lines):
```
Linking .pio/build/esp32s3-cam/firmware.elf
Building .pio/build/esp32s3-cam/firmware.bin
RAM:   [====      ]  38.4% (used 125648 bytes from 327680 bytes)
Flash: [=====     ]  50.1% (used 1314764 bytes from 2621440 bytes)
```

If the build fails, check `Build Messages` in the PlatformIO terminal for
the first error line.

---

## 3. Configure WiFi Credentials

Edit `src/parameters.h` (or after first flash, use the web interface):

```cpp
// Default AP mode — device creates its own network
#define DEFAULT_WIFI_SSID     "ArduPilot"
#define DEFAULT_WIFI_PASSWORD "ardupilot"
```

To connect to an existing WiFi network on first boot, set `WIFI_MODE = 1`
and provide your SSID/password. If STA connection fails within 60 seconds,
the device falls back to AP mode automatically.

---

## 4. Initial Flash (USB — required once only)

Connect the ESP32-S3-CAM via USB-C. The device appears as a COM port
on Windows.

```powershell
# Flash firmware
pio run -e esp32s3-cam --target upload

# Flash filesystem (web assets, if any)
pio run -e esp32s3-cam --target uploadfs
```

The device reboots after flashing. The serial monitor will show:
```
MavBridge starting...
WiFi STA: connecting to ArduPilot...
WiFi AP: started, SSID=ArduPilot, IP=192.168.4.1
HTTP: listening on port 80
MAVLink bridge ready
```

After Increment 1, **all subsequent firmware updates are done via OTA** — you
will not need to reconnect USB.

---

## 5. Connect and Verify

1. Join the `ArduPilot` WiFi network from your laptop or phone.
2. Open a browser: `http://192.168.4.1/`
3. You should see the status page.
4. Navigate to `http://192.168.4.1/getstatus` — should return JSON with
   `vehicleConnected: false` (no autopilot connected yet).

---

## 6. Connect to ArduPilot Autopilot

| ESP32-S3-CAM Pin | Connect to |
|---|---|
| GPIO17 (TX) | Autopilot RX (Telem port) |
| GPIO18 (RX) | Autopilot TX (Telem port) |
| GND | Autopilot GND |
| 3.3V / 5V | Autopilot 5V or regulated rail |

Configure the autopilot SERIAL port:
- Baud: `921600` (or match `UART_BAUDRATE` parameter)
- Protocol: `MAVLink2`

After a moment, `/getstatus` should show `vehicleConnected: true` and
incrementing packet counts.

---

## 7. Configure Ground Control Station (GCS)

In Mission Planner / QGroundControl:

- Connect type: **UDP**
- Port: `14550`
- Host: `192.168.4.1` (or the device's STA IP if in STA mode)

The bridge will auto-detect the GCS IP from the first incoming UDP packet.

---

## 8. Over-the-Air Firmware Update

After the initial flash, all future builds are deployed via the browser.

```powershell
# Build new firmware
pio run -e esp32s3-cam

# The output binary is at:
# .pio/build/esp32s3-cam/firmware.bin
```

In your browser:
1. Navigate to `http://<device-ip>/update`
2. Click **Choose File** → select `.pio/build/esp32s3-cam/firmware.bin`
3. Click **Update**
4. Wait for the progress bar and reboot (~30 seconds)

**Safety**: The OTA form will refuse to start if the autopilot reports the
vehicle is ARMED. Disarm first.

**Rollback**: If the new firmware crashes before confirming itself, the
bootloader automatically returns to the previous firmware on the next boot.

---

## 9. Changing Parameters at Runtime

Via browser:
```
http://<device-ip>/getParameters   → view all parameters (JSON)
http://<device-ip>/setParameters?UART_BAUDRATE=115200  → set a parameter
```

Via Mission Planner:
Open **MAVLink Inspector** — the bridge forwards MAVLink parameter protocol
commands to and from the autopilot.

---

## 10. Development Workflow Summary

```
┌─────────────────────────────────────────────────────────────┐
│  Developer Laptop                                           │
│                                                             │
│  1. edit code                                               │
│  2. pio run -e esp32s3-cam        (build)                   │
│  3. browser → /update             (OTA push)                │
│  4. monitor serial logs (optional)                          │
│     pio device monitor -b 115200                            │
└─────────────────────────────────────────────────────────────┘
```

Physical USB access is only required for the very first flash.
All increments from Increment 1 onward are deployed wirelessly.

---

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---|---|---|
| Device not appearing as COM port | Driver not installed | Install CP2102/CH340 USB-UART driver |
| WiFi AP not appearing | Boot loop / crash | Hold BOOT button, power cycle to enter download mode, reflash |
| `vehicleConnected: false` | UART wiring or baud mismatch | Check GPIO17/18 wiring; verify baud rate matches autopilot config |
| OTA says "Disarm vehicle first" | Autopilot is armed | Disarm via GCS before starting OTA |
| OTA upload fails midway | Binary too large for OTA slot | Check `.pio/build/esp32s3-cam/firmware.bin` size — must be < 6.25 MB |
| Camera stream not loading | Camera absent or wrong resolution | Check camera connector; try `CAM_RESOLUTION=5` (VGA) |
