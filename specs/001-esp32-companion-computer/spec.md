# Feature Specification: ESP32-S3 Companion Computer for ArduPilot

**Feature Branch**: `001-esp32-companion-computer`
**Created**: 2026-04-19
**Status**: Draft
**Input**: User description: "Build a lightweight companion computer for an STM32 based ardupilot autopilot to manage wifi communication, a camera and provide additional functionality."

## User Scenarios & Testing *(mandatory)*

### User Story 1 — MAVLink WiFi Bridge (Priority: P1)

A drone operator in the field powers on their vehicle. The ESP32-S3 companion
computer starts automatically, connects to a pre-saved wifi network or if not available sets up a wifi access point. The operator
connects their laptop or tablet running Mission Planner or QGroundControl to
that network. From that point forward, all MAVLink telemetry and command traffic
between the autopilot and the ground station flows transparently through the
companion computer without any intervention or configuration. The operator is
unaware the companion computer exists — it behaves like a transparent wireless
link.

**Why this priority**: Without reliable MAVLink bridging, the device has no
value as a companion computer. All other features depend on the WiFi link being
established first. This is the safety-critical core of the system.

**Independent Test**: Can be fully tested with an ESP32-S3, a serial MAVLink
source (real autopilot or MAVLink simulator), and Mission Planner. Success is
confirmed when telemetry appears in Mission Planner and RC override commands
reach the autopilot with no packet loss.

**Acceptance Scenarios**:

1. **Given** the companion computer is powered on, **When** it completes boot,
   **Then** a WiFi access point is visible with the configured SSID within 5
   seconds.
2. **Given** a GCS client is connected to the WiFi AP, **When** the autopilot
   sends a HEARTBEAT message, **Then** the GCS receives the HEARTBEAT within
   10 ms and its connection indicator turns green.
3. **Given** an active GCS connection, **When** the GCS sends a parameter
   request, **Then** the autopilot receives the request unmodified and the GCS
   receives the parameter response unmodified.
4. **Given** an active MAVLink session, **When** the WiFi client disconnects,
   **Then** the autopilot does not receive any spurious or injected MAVLink
   messages and the bridge resumes forwarding when the client reconnects.
5. **Given** the system is running, **When** baud rate is 921600, **Then** no
   MAVLink packet loss occurs at sustained full message rate.

---

### User Story 2 — Live Camera Stream (Priority: P2)

A ground station operator wants to view a live video feed from the vehicle
during operation. They open a web browser on any device connected to the
companion computer's same network or direct to its WiFi access point and navigate to the device's web
interface. A live video stream from the onboard camera is visible in the
browser, updating in real time, without requiring any special software or plugin
installation.

**Why this priority**: Camera visibility significantly increases situational
awareness and is a primary reason for using an ESP32-S3-CAM variant as the
companion computer hardware. It does not affect flight safety if unavailable,
hence P2.

**Independent Test**: Can be tested with the companion computer, camera module,
and a web browser on any WiFi-connected device. Success is confirmed when a
continuously updating live image is displayed.

**Acceptance Scenarios**:

1. **Given** the companion computer is running and a WiFi client is connected,
   **When** the operator navigates to the camera stream URL, **Then** a live
   video feed appears within 3 seconds.
2. **Given** an active camera stream, **When** the operator views the stream on
   a mobile browser (iOS Safari or Android Chrome), **Then** the stream
   displays correctly without plugins or app installation.
3. **Given** an active camera stream and MAVLink bridge both running, **When**
   peak video load occurs, **Then** MAVLink forwarding latency does not exceed
   the 10 ms budget defined in the constitution.
4. Still image capture is **out of scope** — QGroundControl handles snapshot
   capture from its own GCS interface. The companion computer provides live
   streaming only; no download mechanism or storage is required.

---

### User Story 3 — Web-based Telemetry Monitoring & Configuration (Priority: P3)

An operator wants to check the health of the companion computer and the
communication link, and adjust configuration parameters, without needing a full
GCS application. They open the device's web interface in any browser on the
WiFi network. They can see live telemetry summary data (GPS fix, battery
voltage, armed state, link quality), inspect and change system parameters (baud
rate, WiFi channel, UDP ports), and initiate an over-the-air firmware update.

**Why this priority**: Enhances operability but the vehicle can fly with only
the MAVLink bridge (P1) and camera (P2). Configuration is needed for initial
setup and diagnostics, not per-flight.

**Independent Test**: Can be tested with a browser on the WiFi network. Success
is confirmed when all displayed parameters match actual system state and
changes persist across a power cycle.

**Acceptance Scenarios**:

1. **Given** the operator opens the web interface root URL, **When** the page
   loads, **Then** a live telemetry summary (armed state, battery voltage, GPS
   fix, MAVLink link quality) is visible and updates at least once per second.
2. **Given** the operator navigates to the parameters page, **When** they change
   a parameter value (e.g., UDP port) and save, **Then** the new value persists
   after a device reboot.
3. **Given** the operator uploads a firmware binary via the OTA update page,
   **When** the upload completes, **Then** the device reboots into the new
   firmware and the web interface becomes accessible again within 30 seconds.
4. **Given** any web page is loading, **When** the response takes longer than
   500 ms, **Then** a loading indicator is shown to the operator.
5. **Given** the operator submits an invalid parameter value (out of range),
   **When** the form is submitted, **Then** a clear, plain-English error message
   is displayed and the invalid value is not applied.

---

### User Story 4 — Motor Driver & Signal Processing (Priority: P4)

A vehicle integrator has connected the companion computer to a ZS-X11H
hoverboard motor controller. ArduPilot sends servo output channel values via
MAVLink. The companion computer intercepts the `SERVO_OUTPUT_RAW` message,
extracts the configured channel value, converts it to a 490 Hz PWM signal on
the appropriate GPIO pin, and drives the motor controller. A failsafe stops the
motor if MAVLink signal is lost for more than 1 second.

**Why this priority**: This is specialised functionality that applies to a
specific vehicle configuration. It does not affect operators using the device
as a pure WiFi bridge or camera platform. It is last priority but must not
degrade any P1–P3 functionality when active.

**Independent Test**: Can be tested with a MAVLink simulator generating
`SERVO_OUTPUT_RAW` and an oscilloscope or logic analyser verifying the PWM
output frequency and duty cycle. A physical ZS-X11H test is required before
vehicle integration.

**Acceptance Scenarios**:

1. **Given** the motor driver feature is enabled and `SERVO_OUTPUT_RAW` is
   being received, **When** channel value is 1500 µs (50%), **Then** the PWM
   output duty cycle is 50% at 490 Hz ± 10 Hz.
2. **Given** active PWM output, **When** no MAVLink message is received for
   more than 1 second, **Then** the PWM output is set to 0% (motor stop) until
   MAVLink resumes.
3. **Given** the motor driver and MAVLink bridge are both active, **When** the
   bridge is forwarding messages at full rate, **Then** PWM output timing is
   not disrupted.
4. **Given** the motor driver is not enabled in configuration, **When** the
   system boots, **Then** no PWM signal is generated on the motor driver GPIO
   and the GPIO remains in a safe tri-state or low-output state.

---

### Edge Cases

- What happens when the autopilot UART is disconnected after boot — does the
  WiFi AP remain active and the web interface accessible?
- How does the system behave when multiple GCS clients connect simultaneously
  to the same UDP port?
- What happens when the camera module is physically absent — does the web
  interface still load without error?
- What happens if a firmware OTA upload is interrupted mid-transfer?
- What happens when the companion computer is the only powered device (autopilot
  off) — does it boot safely without MAVLink input?

## Requirements *(mandatory)*

### Functional Requirements

**MAVLink Bridge**
- **FR-001**: The system MUST forward all MAVLink V2 messages between the
  autopilot UART and connected UDP GCS clients without modification to message
  content, sequence numbers, or checksums.
- **FR-002**: The system MUST create a WiFi access point with a configurable
  SSID and password at boot.
- **FR-003**: The system MUST support simultaneous connection from at least one
  GCS client via UDP.
- **FR-004**: The system MUST expose UART baud rate, WiFi channel, and UDP port
  numbers as configurable parameters.
- **FR-005**: The system MUST log MAVLink message counts (sent, received,
  errors) accessible via the web interface.

**Camera**
- **FR-006**: The system MUST stream live MJPEG video from the ESP32-S3-CAM
  camera module, accessible at a fixed URL over the WiFi network.
- **FR-007**: The camera stream MUST be viewable in a standard browser without
  plugins on iOS, Android, and desktop platforms.
- **FR-008**: The camera stream MUST NOT degrade MAVLink forwarding beyond the
  performance budgets defined in the constitution.

**Web Interface**
- **FR-009**: The web interface MUST display live telemetry summary values
  derived from received MAVLink messages (armed state, battery voltage, GPS
  fix, link quality) updating at least once per second.
- **FR-010**: The web interface MUST allow reading and writing of all system
  parameters with changes persisting across reboots.
- **FR-011**: The web interface MUST support OTA firmware update via file upload.
- **FR-012**: All web interface status indicators MUST use the convention:
  green = nominal, amber = degraded, red = fault.
- **FR-013**: All web forms MUST validate input client-side before submission
  and display plain-English error messages for invalid values.

**Motor Driver**
- **FR-014**: The system MUST optionally generate a 490 Hz PWM signal on a
  configurable GPIO pin, with duty cycle derived from a configurable
  `SERVO_OUTPUT_RAW` channel (value range 1000–2000 µs mapped to 0–100%).
- **FR-015**: The motor driver MUST apply a failsafe (0% duty cycle) if no
  `SERVO_OUTPUT_RAW` message is received within 1 second.
- **FR-016**: The motor driver feature MUST be independently enable/disable via
  a system parameter without requiring firmware reflash.

### Key Entities

- **MAVLink Message**: A framed V2 protocol packet received from the autopilot
  UART or a GCS UDP client; has a message ID, system ID, component ID, payload,
  and CRC.
- **System Parameter**: A named, typed, persistent configuration value stored in
  non-volatile memory; read and written via both the MAVLink parameter protocol
  and the web interface.
- **GCS Client**: A UDP endpoint representing a connected ground control station
  application; identified by IP address and port.
- **Camera Frame**: A JPEG-encoded image captured from the camera module;
  delivered to web clients as part of a multipart MJPEG stream.
- **PWM Output**: A digital signal at a fixed frequency with variable duty
  cycle, generated on a GPIO pin to drive the motor controller.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A GCS client receives its first HEARTBEAT from the autopilot
  within 5 seconds of connecting to the WiFi AP.
- **SC-002**: MAVLink end-to-end forwarding latency (UART in to UDP out) is
  below 10 ms at 921600 baud sustained, measured with no packet loss.
- **SC-003**: The live camera stream appears in a browser within 3 seconds of
  navigating to the stream URL, on both mobile and desktop browsers.
- **SC-004**: Web interface pages load and display current data within 500 ms
  under normal operating conditions.
- **SC-005**: A parameter value change made via the web interface persists
  correctly after a full power cycle.
- **SC-006**: PWM output duty cycle is within ±2% of the target value derived
  from the servo channel input across the full 1000–2000 µs range.
- **SC-007**: The motor failsafe engages (output drops to 0%) within 1.1 seconds
  of MAVLink signal loss.
- **SC-008**: Free heap at steady state (all features active) remains above
  20 KB, measured by monitoring heap over a 10-minute continuous operation
  period.
- **SC-009**: The firmware binary size does not exceed 1.5 MB.

## Assumptions

- The target hardware is exclusively the ESP32-S3-CAM module with PSRAM; no
  other hardware variants are in scope for this feature.
- The autopilot communicates using MAVLink V2 with the ArduPilot dialect over
  UART at up to 921600 baud.
- The WiFi access point mode is the primary operating mode; station (STA) mode
  is available as a configurable option but is not required for the initial
  implementation.
- Camera streaming uses RTSP.
- Still image capture is out of scope. QGroundControl handles snapshot capture
  via its own GCS interface; the companion computer provides live streaming only.
- The ZS-X11H motor driver integration targets a single motor output channel;
  multi-channel PWM output is out of scope.
- OTA updates use the ESP32 built-in update mechanism; a working WiFi connection
  to the device is required to perform an update.
- The web interface is served directly from the companion computer; no external
  server or internet connection is required.
- Standard GCS compatibility targets Mission Planner, QGroundControl, and
  MAVProxy; other GCS applications are best-effort.
