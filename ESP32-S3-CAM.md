# ESP32-S3-CAM Board Support

This document describes the ESP32-S3-CAM 40-pin camera development board and its integration into the mavesp project.

## Board Overview

The ESP32-S3-CAM is a powerful development board based on the ESP32-S3 dual-core processor, featuring:

- **MCU**: ESP32-S3 dual-core Xtensa LX7 @ 240MHz
- **Flash**: 16MB (N16R8)
- **PSRAM**: 8MB OPI PSRAM
- **Camera Interface**: OV2640/OV5640 support via 8-bit parallel interface
- **SD Card**: 4-bit SDMMC interface
- **Connectivity**: WiFi 802.11 b/g/n, Bluetooth 5.0 (LE)
- **USB**: Native USB support (GPIO19/20)
- **Form Factor**: 40-pin development board

## Pin Configuration

All pin definitions are in the Arduino variant file `variants/esp32s3-cam/pins_arduino.h`. The board configuration is defined in `boards/esp32s3-cam.json`.

The board is mounted on the freenove esp32-s3 breakout board that is labelled with the GPIO numbers (https://github.com/Freenove/Freenove_Breakout_Board_for_ESP32/tree/main)

### MAVLink UART Pins

For MAVLink communication, the firmware uses Serial1 on ESP32:

- **TX Pin**: GPIO17
- **RX Pin**: GPIO18
- **Default Baud**: 921600

**Note**: These pins are not used by camera or SD card, making them ideal for MAVLink communication.

### Camera Interface (OV2640/OV5640)

| Signal | GPIO |
|--------|------|
| XCLK   | 10   |
| SIOD   | 40   |
| SIOC   | 39   |
| D7     | 48   |
| D6     | 11   |
| D5     | 12   |
| D4     | 14   |
| D3     | 16   |
| D2     | 15   |
| D1     | 13   |
| D0     | 21   |
| VSYNC  | 38   |
| HREF   | 47   |
| PCLK   | 45   |

### SD Card Interface

| Signal | GPIO |
|--------|------|
| CLK    | 41   |
| CMD    | 42   |
| D0     | 3    |
| D1     | 4    |
| D2     | 5    |
| D3     | 6    |

### LED Pins

There is a WS2812 attached to GPIO48.

### Available GPIO Pins

The following GPIO pins are not used by camera or SD card and are available for general use:

- GPIO1, GPIO2: Alternative UART
- GPIO8, GPIO9: General purpose
- GPIO17, GPIO18: **Recommended for MAVLink** (UART1)
- GPIO19, GPIO20: USB D-/D+ (also available as GPIO)
- GPIO46: General purpose (strapping pin, use with care)

## Building for ESP32-S3-CAM

### PlatformIO Configuration

The board configuration is already set up in `platformio.ini` as the default environment. To build:

```bash
pio run
```

To upload firmware:

```bash
pio run -t upload
```

You can also explicitly specify the environment:

```bash
pio run -e esp32s3-cam -t upload
```

### Environment Settings

The `esp32s3-cam` environment includes:

- Custom board definition in `boards/` directory
- 16MB flash (N16R8) with 8MB PSRAM support
- PSRAM cache fix enabled
- 240MHz CPU frequency
- 80MHz flash frequency
- QIO flash mode
- 921600 baud upload speed

## Usage with MAVLink Bridge

### Wiring ArduPilot/PX4 to ESP32-S3-CAM

Connect your flight controller to the ESP32-S3-CAM as follows:

| Flight Controller | ESP32-S3-CAM |  |
|-------------------|--------------|--|
| TELEM TX          | GPIO18 (RX)  |  |
| TELEM RX          | GPIO17 (TX)  |  |
| GND               | GND          |  |
| 5V (optional)     | 5V           |  |

**Note**: The ESP32-S3-CAM can be powered via USB-C or through the 5V pin. Ensure voltage levels are compatible (3.3V logic).
- **WiFi Mode**: AP (Access Point)
- **Default SSID**: MavEsp8266-XXX (where XXX is last octet of IP)
- **UDP Ports**: 14550 (host), 14555 (client)

## Important Notes

### Strapping Pins

The following pins have special functions during boot and should be used carefully:

- **GPIO0**: Boot mode selection
- **GPIO3**: JTAG enable
- **GPIO45**: VDD_SPI voltage selection
- **GPIO46**: ROM messages enable

### PSRAM

The board includes 8MB of PSRAM with OPI (Octal Peripheral Interface) support. The PSRAM cache issue fix is enabled by default in the build flags.

### USB Support

The ESP32-S3 has native USB support on GPIO19 (USB D-) and GPIO20 (USB D+). These can be used for:
- USB Serial debugging (alternative to UART0)
- USB CDC (Communication Device Class)
- USB MSC (Mass Storage Class)

## Troubleshooting

### Upload Issues

If you experience upload issues:

1. Press and hold BOOT button
2. Press RESET button briefly
3. Release BOOT button
4. Start upload within 5 seconds

Alternatively, reduce upload speed in `platformio.ini`:

```ini
upload_speed = 115200
```

### Serial Communication

The serial ports configuration:
- **Serial (UART0)**: GPIO43 (TX), GPIO44 (RX) - USB Serial for debugging
- **Serial1 (UART1)**: GPIO17 (TX), GPIO18 (RX) - Used for MAVLink communication

### WiFi Range

The ESP32-S3 WiFi is configured for maximum power output (19.5 dBm). If you experience connectivity issues:

- Ensure antenna is properly connected
- Check for interference on WiFi channel 11 (default)
- Modify WiFi channel in parameters if needed

## Compatible Boards

This configuration is designed for ESP32-S3 N16R8 variant (16MB Flash, 8MB PSRAM) with 40-pin layout, including:
- Generic ESP32-S3-CAM N16R8 modules
- Freenove ESP32-S3-WROOM CAM Board (N16R8)
- AI-Thinker ESP32-S3-CAM (N16R8)
- And other compatible 40-pin ESP32-S3 N16R8 camera boards

**Note**: For boards with 8MB flash (N8R8), change `board_build.flash_size` to `8MB` and use `default_8MB.csv` partitions in `platformio.ini`.

## References

- [ESP32-S3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [PlatformIO ESP32 Platform Documentation](https://docs.platformio.org/en/latest/platforms/espressif32.html)

## Support

For issues specific to the ESP32-S3-CAM board support in mavesp, please open an issue on the project repository.

---

**Board Definition Files:**
- Board JSON: `boards/esp32s3-cam.json`
- Pin Definitions: `variants/esp32s3-cam/pins_arduino.h`
- Partition Table: `partitions.csv`
- PlatformIO Environment: `platformio.ini` → `[env:esp32s3-cam]`
