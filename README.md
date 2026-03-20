# ArduPilot ESP32 Webserver

## ESP32-S3-CAM WiFi Access Point, MavLink Bridge & Web Server

This project is an enhanced version of the original MavESP8266, specifically designed for ESP32-S3-CAM modules with integrated webserver capabilities and advanced MAVLink processing features. It provides a robust communication bridge between ArduPilot autopilots and ground control stations while offering web-based monitoring and control interfaces.

### Key Features

- **ESP32-S3-CAM Platform**: Optimized for ESP32-S3-CAM with PSRAM support
- **MAVLink V2 Bridge**: Transparent WiFi-to-UART MAVLink message routing
- **Integrated Web Server**: Real-time telemetry monitoring and parameter configuration
- **PPM to PWM Conversion**: Advanced servo/motor control signal processing
- **High-Speed Communication**: Stable operation at 921600 baud
- **WiFi Access Point**: Creates isolated network for field operations
- **Parameter Management**: Web-based configuration of system parameters

This project has been tested extensively with ArduPilot-based autopilots and ground control stations including Mission Planner, QGroundControl, and MAVProxy.

## Build Environment

The build environment uses [PlatformIO](http://platformio.org) with the ESP32-S3-CAM as the default target. Follow the installation instructions at http://platformio.org/#!/get-started.

### Quick Start

```bash
# Install PlatformIO (if not already installed)
pip install platformio

# Clone the repository with MAVLink submodule
git clone --recursive https://github.com/YourUsername/ardupilot-webserver.git
cd ardupilot-webserver

# Build for ESP32-S3-CAM (default target)
platformio run

# Build and upload to ESP32-S3-CAM
platformio run -t upload

# Monitor serial output
platformio run -t monitor
```

When you run `platformio run` for the first time, it will automatically download the ESP32 toolchain and required libraries.

### Build Targets

- `esp32s3-cam` - ESP32-S3-CAM (default, with PSRAM support)
- `esp12e` - Legacy ESP12e support  
- `esp01_1m` - ESP-01 with 1MB flash
- `esp01` - Basic ESP-01

### Useful Commands

* `platformio run` - Build all targets
* `platformio run -e esp32s3-cam` - Build ESP32-S3-CAM target only
* `platformio run -e esp32s3-cam -t upload` - Build and upload to ESP32-S3-CAM
* `platformio run -t clean` - Clean build files
* `platformio run -t monitor` - Monitor serial output

## MAVLink Integration Overview

### Library Structure
MAVLink is integrated as a **git submodule** pointing to the official MAVLink C Library v2:
- **Submodule path**: `lib/mavlink/` 
- **Source**: https://github.com/mavlink/c_library_v2.git
- **Protocol**: MAVLink V2 with ArduPilot dialect support

### Communication Architecture

The ESP32 acts as a **MAVLink bridge** between your autopilot and ground control station:

```
[Vehicle/Autopilot] ←→ UART ←→ [ESP32-S3 Bridge] ←→ WiFi/UDP ←→ [Ground Control Station]
                                      ↕
                                [Web Interface]
```

### Protocol Configuration

The project uses the **ArduPilot MAVLink dialect**, providing:
- **Core MAVLink**: Common message set (`lib/mavlink/common/`)
- **ArduPilot Extensions**: ArduPilot-specific messages (`lib/mavlink/ardupilotmega/`)
- **Main Include**: `#include <ardupilotmega/mavlink.h>`

### Key Components

#### Vehicle Interface (`mavesp8266_vehicle.cpp`)
- **Function**: Communicates with autopilot via Serial UART
- **ESP32-S3 Pins**: GPIO43 (TX) / GPIO44 (RX) - USB Serial/CDC
- **Baud Rate**: Configurable (default: 921600 bps)
- **Buffer Size**: 4096 bytes for high-throughput communication
- **Message Flow**: Autopilot → ESP32 → GCS

#### GCS Interface (`mavesp8266_gcs.cpp`)  
- **Function**: Handles WiFi communication with ground control stations
- **Protocol**: UDP over WiFi
- **Default Ports**: 14550 (host), 14555 (client)
- **Message Flow**: GCS → ESP32 → Autopilot

#### Web Server Interface (`mavesp8266_httpd.cpp`)
- **Function**: Provides web-based configuration and monitoring
- **Features**: Parameter management, telemetry display, firmware updates
- **Access**: via WiFi access point (typically http://192.168.4.1)

### Message Processing

The system processes critical MAVLink message types including:

- **HEARTBEAT** (`MAVLINK_MSG_ID_HEARTBEAT`): System status and presence detection
- **PARAM_SET** (`MAVLINK_MSG_ID_PARAM_SET`): Parameter configuration management  
- **COMMAND_LONG** (`MAVLINK_MSG_ID_COMMAND_LONG`): Command execution requests
- **SERVO_OUTPUT_RAW** (`MAVLINK_MSG_ID_SERVO_OUTPUT_RAW`): PWM servo control data
- **PARAM_REQUEST_LIST** (`MAVLINK_MSG_ID_PARAM_REQUEST_LIST`): Parameter discovery
- **PARAM_REQUEST_READ** (`MAVLINK_MSG_ID_PARAM_REQUEST_READ`): Individual parameter requests

### Communication Channels
- **MAVLINK_COMM_0**: Vehicle ↔ ESP32 communication channel
- **MAVLINK_COMM_1**: ESP32 ↔ GCS communication channel
- **Dual Channel**: Allows simultaneous bidirectional communication

### PPM to PWM Conversion Feature

The `feature-convert_PPM_to_PWM` branch includes advanced servo control capabilities:
- **Input**: PPM (Pulse Position Modulation) signal streams
- **Output**: Individual PWM (Pulse Width Modulation) signals
- **Use Case**: Direct servo/motor control for custom actuator configurations
- **Integration**: Processes `MAVLINK_MSG_ID_SERVO_OUTPUT_RAW` messages for real-time control

## Hardware Configuration

### ESP32-S3-CAM Module
This project is optimized for ESP32-S3-CAM development boards featuring:
- **ESP32-S3** dual-core processor with WiFi support
- **PSRAM**: 8MB external RAM for enhanced performance
- **Camera Interface**: OV2640 camera (optional for future features)  
- **MicroSD Slot**: For data logging and configuration storage
- **GPIO Access**: Multiple pins available for PPM/PWM interfaces

### Wiring Connections

#### Standard UART Connection (Autopilot ↔ ESP32-S3)
- **ESP32-S3 GPIO43 (TX)** → **Autopilot RX** (SERIAL1_TX or TELEM1 RX)
- **ESP32-S3 GPIO44 (RX)** → **Autopilot TX** (SERIAL1_RX or TELEM1 TX)  
- **GND** → **GND** (common ground connection)
- **5V or 3.3V** → **Appropriate power rail** (check your autopilot specifications)

#### PPM/PWM Extensions (for servo control features)
- Additional GPIO pins can be configured for PPM input or PWM output
- Refer to `freenove_esp32s3_pin_mapping.csv` for detailed pin assignments
- Custom pin configurations available in `variants/esp32s3-cam/pins_arduino.h`

### Power Requirements
- **Input Voltage**: 5V via USB-C or 3.3V via pin headers
- **Power Consumption**: ~200-300mA during operation
- **WiFi Range**: 50-100m in open field conditions

## Web Interface

The integrated web server provides comprehensive control and monitoring capabilities:

### Access Methods
1. **WiFi Access Point Mode** (default):
   - **SSID**: `ArduPilot-ESP32` (configurable)
   - **IP Address**: http://192.168.4.1
   - **Default Password**: `ardupilot123` (change immediately)

2. **Station Mode** (connect to existing network):
   - Configure via web interface or parameters
   - Access via assigned IP address from your router

### Web Interface Features
- **Real-time Telemetry**: Live MAVLink message monitoring
- **Parameter Configuration**: Modify ESP32 bridge parameters
- **Firmware Updates**: Over-the-air firmware upload capability
- **Network Settings**: WiFi configuration and network management
- **System Status**: Connection status, message statistics, and diagnostics
- **Log Download**: Access system logs and diagnostic information

### API Endpoints
- `GET /` - Main web interface
- `GET /status` - JSON system status
- `GET /parameters` - Current parameter values  
- `POST /parameters` - Update parameter values
- `POST /firmware` - Firmware upload endpoint

## MAVLink Submodule Management

The MAVLink library is included as a git submodule. To update or maintain the MAVLink definitions:

```bash
# Initialize submodules (if not done during clone)
git submodule update --init

# Update MAVLink to latest version
git submodule update --remote lib/mavlink

# Check submodule status
git submodule status
```

The current implementation uses MAVLink C Library v2 with ArduPilot message definitions, ensuring compatibility with all ArduPilot-based autopilots including:
- **Pixhawk series**: Pixhawk 1-6, CubeBlack, CubeOrange
- **ArduPilot flight controllers**: Matek, Holybro, mRo controllers  
- **Custom builds**: Any ArduPilot-compatible hardware

## Configuration & Parameters

### System Parameters
The ESP32 bridge maintains its own parameter set accessible via:
- **Web interface**: http://192.168.4.1/parameters
- **MAVLink**: Standard parameter request/set messages
- **Serial console**: Direct parameter access via USB

### Key Configuration Options
- **UART Baud Rate**: Communication speed with autopilot (default: 921600)
- **WiFi Channel**: Operating frequency (default: 11)
- **UDP Ports**: GCS communication ports (14550/14555)
- **WiFi Credentials**: SSID and password for station mode
- **PPM Configuration**: Input pin and timing for PPM to PWM conversion

### Reset to Defaults
**GPIO02 Reset**: Connect GPIO02 to GND during power-up to reset all parameters to factory defaults.

## Troubleshooting

### Common Issues

**Communication Problems**:
- Verify UART wiring and baud rate matching
- Check autopilot SERIAL port configuration
- Confirm 3.3V/5V compatibility between devices

**WiFi Connectivity**:
- Ensure ESP32 access point is active (check LED indicators)
- Verify client device WiFi settings
- Check for interference on WiFi channel

**Web Interface Access**:
- Confirm connection to ESP32 access point
- Try alternative browsers if interface doesn't load
- Check for firewall blocking local network access

**Build Issues**:
- Update PlatformIO: `pip install --upgrade platformio`
- Clean build: `platformio run -t clean`
- If enum errors occur: `pip3 uninstall -y enum34`

### Serial Diagnostic Output
Connect to serial console at 115200 baud for detailed diagnostic information:
```bash
platformio run -t monitor -e esp32s3-cam
```

## Development & Customization

### Project Structure
```
├── src/                    # Main source code
│   ├── mavesp8266.cpp     # Core application logic
│   ├── mavesp8266_httpd.cpp # Web server implementation
│   ├── mavesp8266_vehicle.cpp # Autopilot communication
│   └── mavesp8266_gcs.cpp  # GCS communication
├── lib/mavlink/           # MAVLink library (submodule)
├── variants/esp32s3-cam/  # ESP32-S3-CAM specific definitions  
├── platformio.ini         # Build configuration
└── partitions.csv         # Flash memory layout
```

### Adding Custom Features
1. **New MAVLink Messages**: Extend message processing in component files
2. **Web Interface**: Modify HTTP handlers in `mavesp8266_httpd.cpp`
3. **GPIO Control**: Add custom pin configurations in variants directory
4. **Parameter Extensions**: Add new parameters in `mavesp8266_parameters.cpp`

## Protocol Documentation

For detailed MAVLink protocol information:
- **Parameters**: See [PARAMETERS.md](PARAMETERS.md)
- **HTTP API**: See [HTTP.md](HTTP.md) 
- **Message Flow**: See [DATAFLOW_DIAGRAM.md](DATAFLOW_DIAGRAM.md)

## License & Credits

Based on the original MavESP8266 project by Gus Grubba, enhanced for ESP32-S3-CAM with webserver capabilities and ArduPilot-specific features.

This project maintains compatibility with the original MavESP8266 parameter structure while extending functionality for modern ESP32 hardware platforms.
