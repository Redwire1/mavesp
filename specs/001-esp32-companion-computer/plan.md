# Implementation Plan: ESP32-S3 Companion Computer for ArduPilot

**Branch**: `001-esp32-companion-computer` | **Date**: 2026-04-21 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/001-esp32-companion-computer/spec.md`

---

## Summary

Transform the existing `mavesp8266`-based MAVLink WiFi bridge into a fully
featured ESP32-S3 companion computer. The device acts as a transparent
MAVLink bridge (vehicle UART ↔ GCS UDP), exposes a web interface for OTA
firmware updates and parameter management, streams live camera video, and
provides a REST API for an LLM companion device (MimiClaw).

**Priority**: OTA update capability is present in the first deployable increment
so that all subsequent development can be deployed remotely without physical
access to the hardware.

**Architecture**: Retain the Arduino `Update`-class OTA mechanism (already
functional), add an armed-state safety gate, and improve the web UI.
Migrate to AsyncWebServer when camera streaming is added in Increment 2.

---

## Technical Context

**Language/Version**: C++17, Arduino framework on PlatformIO (ESP32 Arduino core)
**Primary Dependencies**: `espressif32` platform, Arduino `WebServer` (Increment 1) →
`ESPAsyncWebServer` (Increment 2+), `esp_camera` (Arduino ESP32 bundled), MAVLink v2
(`lib/mavlink/ardupilotmega`), Arduino `Update` class (OTA), LittleFS, LEDC (PWM)
**Storage**: NVS via Arduino EEPROM API (parameters); LittleFS (web assets)
**Testing**: Hardware-in-the-loop (HIL) on physical ESP32-S3-CAM hardware;
manual test procedures per increment; PlatformIO native tests for pure-C++ units
**Target Platform**: Freenove ESP32-S3-CAM (16 MB flash N16R8, 8 MB OPI PSRAM);
PlatformIO `esp32s3-cam` environment
**Project Type**: Embedded firmware (single-project)
**Performance Goals**: MAVLink forwarding latency < 10 ms end-to-end;
firmware binary < 1.5 MB; ≥ 20 KB free SRAM heap at runtime;
web API responses < 500 ms; MJPEG stream ≥ 10 fps at SVGA over WiFi
**Constraints**: 512 KB SRAM (camera frame buffers must use PSRAM);
FreeRTOS task model; Arduino `loop()` on Core 1 for MAVLink;
OTA refused while vehicle is armed; no internet access assumed
**Scale/Scope**: Single embedded device, single GCS client, single LLM companion
client (MimiClaw on a separate ESP32)

---

## Constitution Check

*Constitution version 1.1.0 — evaluated before Phase 0 research and
re-evaluated after Phase 1 design.*

| Principle | Requirement | Status | Notes |
|---|---|---|---|
| I — Safety First | No OTA while vehicle ARMED | ✅ PASS | Armed-state check in `/upload` handler — HTTP 409 if armed |
| I — Safety First | MAVLink bridge must not be interrupted by web tasks | ✅ PASS | FreeRTOS: MAVLink on Core 1 prio 5; web/camera on Core 0 prio ≤ 5 |
| II — Code Quality | No runtime heap allocation after init | ⚠️ EXCEPTION | Camera frame buffers (PSRAM only) allocated per-frame — see Complexity Tracking |
| II — Code Quality | Naming convention enforced | ✅ PASS | All identifiers renamed per constitution in Increment 0 before functional changes |
| III — Test Before Change | Each increment has documented test criteria | ✅ PASS | Test criteria listed per increment below |
| IV — One Concern Per File | New modules separated by concern | ✅ PASS | `ota`, `camera`, `telemetry`, `llm_api` each have a single concern |
| V — Performance Budget | Camera stream must not degrade MAVLink < 10 ms | ✅ PASS | Camera task Core 0 prio 3; MAVLink loop Core 1 prio 5 |

**Post-design re-check**: All gates pass. One justified exception documented below.

---

## Project Structure

### Documentation (this feature)

```text
specs/001-esp32-companion-computer/
├── plan.md              ← this file
├── research.md          ← Phase 0 output
├── data-model.md        ← Phase 1 output
├── quickstart.md        ← Phase 1 output
├── contracts/
│   └── http-api.md      ← Phase 1 output
└── tasks.md             ← Phase 2 output (/speckit.tasks — not yet generated)
```

### Source Code (after Increment 0 rename)

```text
src/
├── main.cpp                # Entry point — exempt from rename
├── bridge.h/.cpp           # Bridge base class + link_status_t
├── component.h/.cpp        # Component — MAVLink param/cmd responder
├── gcs.h/.cpp              # Gcs + gcs_client_t — UDP forwarding
├── vehicle.h/.cpp          # Vehicle — UART forwarding
├── parameters.h/.cpp       # Parameters + parameters_t — NVS persistence
├── httpd.h/.cpp            # Httpd — web server + OTA
├── ppm.h/.cpp              # Ppm — PWM motor driver
├── crc.h                   # Unchanged
├── camera.h/.cpp           # NEW Increment 2 — camera init + MJPEG/RTSP stream
├── telemetry.h/.cpp        # NEW Increment 3 — telemetry_state_t aggregation
├── ota.h/.cpp              # NEW Increment 1 — OTA gate + boot confirmation
└── llm_api.h/.cpp          # NEW Increment 5 — REST API for LLM companion
```

**Structure Decision**: Single-project embedded firmware. Source files in `src/`
flat layout, no prefix. No subdirectories within `src/` — file count
remains manageable.

---

## Complexity Tracking

| Violation | Why Needed | Simpler Alternative Rejected Because |
|---|---|---|
| Runtime PSRAM heap allocation for camera frame buffers | Camera captures variable-size JPEG frames; scene content determines frame size | Pre-allocating a fixed max frame buffer wastes ~300 KB of PSRAM per buffer (×2 double-buffer = 600 KB). PSRAM-only allocation via `heap_caps_malloc(MALLOC_CAP_SPIRAM)` is the canonical `esp_camera` pattern and does not risk SRAM fragmentation. |

---

## Development Increments

> **User requirement**: OTA update capability must be present in Increment 1
> (the first deployable build) so that all subsequent increments are deployed
> wirelessly without physical USB access.

---

### Increment 0 — Foundation (NOT deployable — local rename only)

**Goal**: Rename all files and class identifiers per the constitution Naming
Conventions. Zero functional changes. Verify the build succeeds.

**Migration table** (from constitution v1.3.0):

| Old file | New file | Old class/struct | New class/struct |
|---|---|---|---|
| `mavesp8266.h/.cpp` | `bridge.h/.cpp` | `MavESP8266Bridge`, `MavESP8266World` | `Bridge`, `World` |
| `mavesp8266_component.h/.cpp` | `component.h/.cpp` | `MavESP8266Component` | `Component` |
| `mavesp8266_gcs.h/.cpp` | `gcs.h/.cpp` | `MavESP8266GCS` | `Gcs` |
| `mavesp8266_vehicle.h/.cpp` | `vehicle.h/.cpp` | `MavESP8266Vehicle` | `Vehicle` |
| `mavesp8266_parameters.h/.cpp` | `parameters.h/.cpp` | `MavESP8266Parameters`, `stMavEspParameters` | `Parameters`, `parameters_t` |
| `mavesp8266_httpd.h/.cpp` | `httpd.h/.cpp` | `MavESP8266Httpd` | `Httpd` |
| `mavesp8266_ppm.h/.cpp` | `ppm.h/.cpp` | `MavESP8266PPM` | `Ppm` |

Also rename `MavESP8266Log` → `Log`, update all header guards
(`MAVESP8266_*_H` → `*_H`), update all `#include` directives and
`main.cpp` references. Method and member names are already conformant —
no function renames are required.

**Test criteria (Increment 0)**:
- `pio run -e esp32s3-cam` compiles with 0 errors, 0 warnings
- Existing OTA and MAVLink forwarding work correctly after reflash

---

### Increment 1 — Deployable MVP: MAVLink Bridge + OTA

**Goal**: First build flashed physically to the device. After this, all further
work is deployed via OTA.

**Scope**:

1. **`ota.h/.cpp`** (new):
   - `otaBegin()`: checks `is_armed` → returns error if true
   - `otaWrite(data, len)`: wraps `Update.write()`
   - `otaEnd()`: wraps `Update.end()`, calls boot confirmation
   - Boot confirmation: `esp_ota_mark_app_valid_cancel_rollback()` within 30 s
     of successful WiFi connection and first web request

2. **`httpd.cpp`** (update OTA endpoints):
   - `GET /update`: return 409 + error page if armed; else serve upload form
   - `POST /upload`: call `otaBegin()` before `Update.begin()`, propagate 409
   - Progress tracking on upload form
   - `GET /getstatus`: add `is_armed` field to JSON

3. **`parameters.cpp`**: Add `LLM_API_ENABLED`, `LLM_API_SECRET`, `LLM_PUSH_URL`,
   `LLM_THRESH_BATT`, `CAM_RESOLUTION`, `CAM_QUALITY`, `PWM_ENABLED` (all
   disabled; NVS space reserved for future increments)

4. **MAVLink bridge** (verify after rename): UART2 at 921600 baud GPIO17/18;
   UDP port 14550 (in), 14555 (out); param get/set/request_list; heartbeat;
   STA→AP WiFi fallback with 60 s timeout

**New PlatformIO libraries**: None (all required libraries already present).

**Test criteria (Increment 1)**:
- OTA upload succeeds from browser while vehicle is disarmed; device reboots
  and new firmware persists across subsequent power cycles (no rollback)
- OTA upload returns HTTP 409 while HEARTBEAT shows ARMED
- MAVLink packets forwarded bidirectionally; Mission Planner HUD shows live data
- `/getstatus` returns correct `is_armed` field

---

### Increment 2 — Camera Streaming (OTA push)

**Goal**: Add live camera video stream compatible with QGroundControl and
directly viewable in a browser.

**Streaming architecture**:

| Sink | Protocol | Port / URL | Consumer |
|---|---|---|---|
| RTSP server | RTSP/RTP MJPEG | 554, `/mjpeg/1` | QGroundControl, VLC |
| HTTP endpoint | multipart MJPEG | 80 `/stream` | Browser direct access |

Both sinks consume the same JPEG frames — no re-encoding.

**QGC setup**: Video Source = `RTSP Video Stream`,
URL = `rtsp://<device-ip>:554/mjpeg/1`.

**Scope**:

2. **Add PlatformIO dependencies**:
   - `ESP Async WebServer`
   - `Micro-RTSP` (geeksville)

3. **`camera.h/.cpp`** (new):
   - `cameraInit(camera_config_t)`: calls `esp_camera_init()` with OV2640 pins
   - `cameraStreamTask(void*)`: FreeRTOS task on Core 0, priority 3;
     captures JPEG frame → distributes to RTSP sink and HTTP sink →
     returns frame buffer
   - RTSP sink: passes frame to `MjpegConnectedSession::r(sessionList,...)`
   - HTTP sink: writes frame to open `AsyncResponseStream` if client connected
   - Stops gracefully on all client disconnects

4. **`httpd.cpp`** (migrate to AsyncWebServer):
   - Replace synchronous `WebServer` with `AsyncWebServer` on port 80
   - All existing endpoint handlers rewritten as async lambdas
   - `GET /stream`: MJPEG multipart response for browser access

5. **RTSP server** (`camera.cpp`):
   - `CStreamer` / `CRtspSession` from Micro-RTSP on port 554
   - Accepts connections from QGC/VLC
   - Single concurrent RTSP session (Micro-RTSP default); additional clients
     via the HTTP `/stream` endpoint

**Test criteria (Increment 2)**:
- QGC displays live video: Settings → Video Source = RTSP, URL =
  `rtsp://<device-ip>:554/mjpeg/1`
- Browser displays live video at `http://<device-ip>/stream`
  (Chrome, Safari, Firefox without plugin)
- MAVLink latency < 10 ms while stream active on both sinks simultaneously
- Existing OTA and parameter endpoints functional after AsyncWebServer migration
- No PSRAM memory leak on repeated RTSP connect/disconnect cycles

---

### Increment 3 — Web Telemetry Dashboard (OTA push)

**Goal**: Full web dashboard with live telemetry and parameter management UI.

**Scope**:

1. **`telemetry.h/.cpp`** (new): Parses `HEARTBEAT`, `SYS_STATUS`, `GPS_RAW_INT`,
   `VFR_HUD`, `GLOBAL_POSITION_INT` from MAVLink stream; populates
   `telemetry_state_t`; provides `telemetryGetJson()` from a static buffer

2. **`httpd.cpp`** (extend): `GET /status` serves full dashboard HTML;
   `GET /getstatus` extended with full telemetry fields

3. **Web UI assets** (LittleFS `data/`): `index.html` with green/amber/red
   status indicators; `params.html` for parameter management; JavaScript polling
   at 1 s interval; `pio run --target uploadfs` to push assets independently

**Test criteria (Increment 3)**:
- Dashboard shows correct armed/disarmed, battery %, GPS fix in real time
- Parameter set via web form reflected in `/getParameters` immediately
- Dashboard loads in < 2 s on WiFi

---

### Increment 4 — Motor Driver Integration (OTA push)

**Goal**: Integrate and verify the PPM/PWM motor driver module.

**Scope**: `ppm.cpp` cleanup; verify `SERVO_OUTPUT_RAW` channel routing;
test failsafe (0% PWM on MAVLink loss > 1 s); add `PWM_ENABLED` parameter gate.

**Test criteria (Increment 4)**:
- Motor responds to `SERVO_OUTPUT_RAW` with correct PWM duty
- Failsafe triggers when UART disconnected > 1 s
- `PWM_ENABLED = 0` does not init LEDC peripheral

---

### Increment 5 — LLM Companion API (OTA push)

**Goal**: Expose authenticated REST API for the MimiClaw LLM companion device.

**Scope**:

1. **`llm_api.h/.cpp`** (new):
   - Registers AsyncWebServer handlers for `/api/v1/telemetry` and
     `/api/v1/command`
   - `X-Api-Key` validated with constant-time comparison before any handler runs
   - Command allowlist: `SET_PARAMETER`, `REQUEST_MODE`
   - Outbound event push task (Core 0, prio 2): checks thresholds every 5 s;
     POSTs `llm_event_t` JSON to `LLM_PUSH_URL`; de-bounced per threshold crossing
   - All responses use static JSON buffers — no heap allocation

2. **Security controls**:
   - `memcmp`-based key comparison (no early-exit timing leak)
   - No string interpolation into MAVLink payload
   - All `/api/v1/` return 404 when `LLM_API_ENABLED = 0`

**Test criteria (Increment 5)**:
- `/api/v1/telemetry` returns correct vehicle state with valid key; 401 on wrong key
- `POST /api/v1/command` with `SET_PARAMETER` updates parameter value
- Unknown command type returns 400; read-only param returns 422
- Event push fires once on battery threshold crossing; does not re-fire until
  battery recovers and re-crosses

---

## Design Artifacts

| Artifact | File | Status |
|---|---|---|
| Phase 0 research | [research.md](research.md) | ✅ Complete |
| Data model | [data-model.md](data-model.md) | ✅ Complete |
| HTTP API contract | [contracts/http-api.md](contracts/http-api.md) | ✅ Complete |
| Developer quick start | [quickstart.md](quickstart.md) | ✅ Complete |
| Task breakdown | tasks.md | ⬜ Pending — run `/speckit.tasks` |
