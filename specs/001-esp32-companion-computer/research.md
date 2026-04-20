# Research: ESP32-S3 Companion Computer for ArduPilot

**Branch**: `001-esp32-companion-computer`
**Date**: 2026-04-21
**Status**: Complete — all NEEDS CLARIFICATION resolved

---

## 1. OTA Update Mechanism

**Decision**: Retain the Arduino `Update` class (`<Update.h>`) already used in
`mavesp8266_httpd.cpp`. Supplement it with an armed-state safety gate and a
clean boot-confirmation handshake.

**Rationale**: The `Update` class on ESP32 Arduino is a thin wrapper around the
ESP-IDF `esp_ota_ops` API. It writes to the inactive OTA partition slot
(`app0`/`app1`), marks it bootable, and reboots. The existing `partitions.csv`
is correctly configured with dual 6.25 MB OTA slots and a `coredump` region.
No changes to the partition table are required.

**Safety gate (Constitution I)**: OTA must refuse to begin if the autopilot
HEARTBEAT indicates ARMED state. The `telemetry_state_t` struct will expose an
`is_armed` flag; the upload handler checks this before calling `Update.begin()`.
If armed, HTTP 409 Conflict is returned with the message
`"Disarm vehicle before updating firmware"`.

**Boot confirmation**: After OTA reboot the new firmware confirms itself with
`esp_ota_mark_app_valid_cancel_rollback()` (via the Arduino ESP32 framework
wrapper) within 30 seconds of successful WiFi connection and first web request.
If it never confirms, the bootloader auto-rolls back to the previous slot on
the next power cycle.

**Remote deployment path**: With OTA functional from Increment 1, every
subsequent increment is deployed wirelessly. Physical USB access is only
required for the initial flash.

**Alternatives considered**:
- `esp_https_ota`: requires an external firmware server; adds internet
  dependency; rejected.
- ArduinoOTA (network): uses UDP; does not support the browser-based workflow
  needed for field updates from a mobile device; rejected.

---

## 2. HTTP Server Framework

**Decision**: Migrate from the synchronous Arduino `WebServer` to
`AsyncWebServer` (the ESPAsyncWebServer library) when camera streaming is
introduced in Increment 2. Increments 0 and 1 retain the existing `WebServer`
to avoid scope creep in the safety-critical MVP.

**Rationale**: The synchronous `WebServer` blocks `loop()` while handling a
request. With MJPEG streaming (multi-part HTTP chunked response held open for
seconds at a time), this would starve the MAVLink forwarding loop, violating the
10 ms latency budget (Constitution V). `AsyncWebServer` runs on an ESP-IDF
HTTP server instance that dispatches callbacks without blocking `loop()`.

The camera stream handler in AsyncWebServer uses `AsyncResponseStream` to push
JPEG frames to connected clients from within a FreeRTOS camera task, entirely
decoupled from the MAVLink bridge.

**WebServer retained in MVP (Increment 1)**: The existing OTA + parameter
endpoints work correctly with `WebServer`. Migrating before the camera is added
is unnecessary complexity.

**Alternatives considered**:
- Mongoose embedded HTTP: capable, but adds a large third-party dependency and
  licensing uncertainty; rejected.
- ESP-IDF `esp_http_server` directly: lower-level than AsyncWebServer with no
  Arduino wrapper; all existing handler code would require rewriting; rejected.

---

## 3. Camera Streaming

**Decision**: Dual-sink delivery from a single OV2640 JPEG frame pipeline:

| Sink | Protocol | Port | Consumer |
|---|---|---|---|
| RTSP server | RTSP/RTP, MJPEG-over-RTP | 554 | QGroundControl, VLC, any RTSP client |
| HTTP multipart | `multipart/x-mixed-replace` | 80 `/stream` | Direct browser access |

**QGC compatibility**: QGroundControl's video system uses GStreamer and supports
these sources: RTSP, UDP H.264, UDP H.265, TCP-MPEG2, MPEG-TS. Plain HTTP
multipart MJPEG is **not** a QGC video source. RTSP is the correct integration
point. QGC configuration: Video Source = `RTSP Video Stream`,
URL = `rtsp://<device-ip>:554/mjpeg/1`.

**Why MJPEG-over-RTSP, not H.264**: The OV2640 sensor outputs JPEG natively.
The ESP32-S3 has a hardware H.264 encoder, but it is not exposed by the Arduino
`esp_camera` library (ESP-IDF only). Encoding JPEG→H.264 in software on the
application CPU would consume too much SRAM and starve the MAVLink bridge.
MJPEG-over-RTSP is directly supported by GStreamer (`rtpjpegpay`) and QGC
decodes it without any special configuration.

**RTSP library**: `Micro-RTSP` by geeksville — a purpose-built, Arduino-
compatible RTSP server for ESP32-CAM hardware, available as a PlatformIO
library (`Micro-RTSP`). It handles RTSP DESCRIBE/SETUP/PLAY/TEARDOWN and
packages each JPEG frame into RTP packets on port 554.

**Browser access**: Browsers do not support RTSP natively. The HTTP `/stream`
endpoint (`multipart/x-mixed-replace; boundary=frame`) provides direct browser
access using the same JPEG frames, with no re-encoding cost. Both sinks pull
from the same `esp_camera_fb_get()` / `esp_camera_fb_return()` cycle.

**FreeRTOS allocation**: Camera capture + stream delivery runs as a persistent
FreeRTOS task on Core 0 at priority 3. MAVLink bridge runs in Arduino `loop()`
on Core 1 at priority 5, ensuring MAVLink always wins CPU contention. Frame
buffers live in PSRAM (`MALLOC_CAP_SPIRAM`) — documented exception to
Principle II (see Complexity Tracking in plan.md).

**Resolution**: OV2640 initialised at SVGA (800×600). `CAM_RESOLUTION` and
`CAM_QUALITY` are runtime parameters.

**Alternatives considered**:
- HTTP multipart MJPEG only: works in browsers but **not visible in QGC**;
  rejected as primary format.
- H.264 via Arduino: hardware encoder not exposed; software encoding kills
  MAVLink performance; rejected.
- UDP H.264 (QGC native): same H.264 encoding blocker; rejected.
- MPEG-TS: more complex framing; no benefit over RTSP for this use case;
  rejected.

---

## 4. WiFi STA / AP Mode Handling

**Decision**: Retain the existing STA-with-AP-fallback logic already implemented
in `main.cpp`. The STA connection timeout (currently 120 × 500 ms = 60 s) is
configurable via a `WIFI_STA_TIMEOUT` parameter.

**Rationale**: The existing implementation correctly:
- Attempts STA connection using stored credentials
- Falls back to AP mode if connection fails within the timeout
- Uses `WiFi.setAutoReconnect(true)` to recover silently from brief disconnects

No changes are required for Increment 1. The fallback AP SSID/password become
runtime parameters (already the case via `MavESP8266Parameters`).

---

## 5. PWM Motor Driver

**Decision**: Retain the existing `mavesp8266_ppm.cpp` module (renamed to
`ppm.cpp` per naming convention). The ESP32-S3 LEDC peripheral generates 490 Hz
PWM with 8-bit resolution, which exactly meets the ZS-X11H requirements.

**Rationale**: The existing module already:
- Intercepts `SERVO_OUTPUT_RAW` from the MAVLink stream
- Maps servo channel value (1000–2000 µs) to LEDC duty cycle (0–255)
- Applies a 1-second failsafe (0% duty) on MAVLink loss

The only required change is a rename and ensuring the failsafe is tested as part
of Increment 4 HIL testing.

---

## 6. FreeRTOS Task Architecture

**Decision**: Adopt a two-core task layout separating real-time MAVLink
forwarding from all I/O-bound tasks.

| Task | Core | Priority | Stack | Role |
|---|---|---|---|---|
| `arduino_loop` (MAVLink bridge) | 1 | 5 | 8 KB | UART↔UDP forwarding, PPM update |
| `camera_stream_task` | 0 | 3 | 8 KB | OV2640 capture + HTTP MJPEG delivery |
| `web_server_task` (AsyncWebServer internal) | 0 | 5 | — | HTTP request dispatch |
| `wifi_event_task` (ESP-IDF internal) | 0 | 8 | — | WiFi event handling |

The MAVLink bridge in `loop()` must not call any blocking I/O. All web server
callbacks are async and must not block.

---

## 7. Parameter Persistence

**Decision**: Continue using the Arduino EEPROM emulation layer (which maps to
NVS under the ESP32 Arduino framework) for all `parameters_t` values. No
migration to LittleFS for parameters is required.

**Rationale**: The existing `MavESP8266Parameters` / `Parameters.saveAllToEeprom()`
pattern is correct and battle-tested. LittleFS will be used only for web UI
assets (HTML/CSS/JS files) if those grow beyond PROGMEM capacity.

---

## 8. LLM Companion API Transport

**Decision**: Expose the companion API as a simple JSON REST interface over
HTTP/1.1 on the existing port 80 (via AsyncWebServer), behind a shared-secret
header (`X-Api-Key`). No WebSocket or persistent connection is required for
Increment 5.

**Rationale**: HTTP GET/POST is the simplest transport compatible with MimiClaw's
existing `web_search` HTTP tool pattern. It avoids the additional FreeRTOS task
and memory overhead of a persistent WebSocket connection for what is essentially
an infrequent query/command pattern. Event push (FR-020) is implemented as an
outbound HTTP POST from a lightweight FreeRTOS task that wakes on threshold
crossings; no persistent connection is held open.

**Security**: Pre-shared key stored in NVS, validated on every request in the
AsyncWebServer request handler before any action is taken. 401 on mismatch.

---

## Summary Table

| Topic | Decision | Key Constraint Satisfied |
|---|---|---|
| OTA mechanism | Arduino `Update` class + armed-state gate | Constitution I (no OTA while armed) |
| HTTP server | WebServer (MVP) → AsyncWebServer (Increment 2+) | Constitution V (10 ms latency) |
| Camera | esp_camera + RTSP (QGC) + HTTP MJPEG (browser) + PSRAM frames | FR-006, FR-008 |
| WiFi | Existing STA/AP fallback, retained | FR-002, SC-001 |
| PWM | Existing LEDC PPM module, renamed to `ppm.cpp` | FR-014, FR-015 |
| FreeRTOS | Core 1 (MAVLink, prio 5) / Core 0 (`cameraStreamTask`, prio 3) | Constitution V |
| Parameters | NVS via Arduino EEPROM API, `parameters_t`, retained | FR-010, SC-005 |
| LLM API | HTTP REST + X-Api-Key header | FR-017–FR-022 |
