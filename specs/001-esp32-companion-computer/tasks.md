# Tasks: ESP32-S3 Companion Computer for ArduPilot

**Input**: Design documents from `specs/001-esp32-companion-computer/`
**Prerequisites**: plan.md ✅, spec.md ✅, research.md ✅, data-model.md ✅, contracts/http-api.md ✅, quickstart.md ✅

**No tests requested** — test criteria are documented per phase as HIL (hardware-in-the-loop) verification steps.

---

## Phase 1: Setup (Pre-flight Verification)

**Purpose**: Verify the existing codebase builds and the hardware prerequisites are
in place before any changes are made. These are gate tasks — nothing in Phase 2
should start until T001–T004 are complete.

- [X] T001 Verify current build: run `pio run -e esp32s3-cam` on unmodified codebase; note any existing warnings to distinguish from regressions introduced in Phase 2
- [X] T002 Verify OTA partition layout: confirm `partitions.csv` has two OTA app slots (ota_0, ota_1) ≥ 6 MB each; search the codebase for any existing call to `esp_ota_mark_app_valid_cancel_rollback()` — if found, **remove it** before proceeding; T023 is the single authoritative call site and any pre-existing call would mark the current (potentially bad) firmware as valid, silently defeating OTA rollback protection
- [X] T003 [P] Research and pin `ESPAsyncWebServer` + `AsyncTCP` library versions compatible with `espressif32` platform version in use; add pinned `lib_deps` entries to `platformio.ini` `[env:esp32s3-cam]` (needed in Phase 4 — pin now to avoid surprise)
- [X] T004 [P] Research and pin `Micro-RTSP` by geeksville to a known-good commit/tag compatible with ESP32 Arduino core 2.x; add pinned entry to `platformio.ini` `lib_deps` (needed in Phase 4)

**Checkpoint**: Build passes, partition layout confirmed, library versions pinned.

---

## Phase 2: Foundational — Increment 0 (Rename & Structural Cleanup)

**Purpose**: Rename all `mavesp8266_*` files, class names, header guards, and the
legacy `linkStatus` struct to match the constitution v1.3.0 naming conventions.
Zero functional changes. This phase MUST be complete and building before ANY user
story work begins.

**⚠️ CRITICAL**: No user story work can begin until T014 (clean build) passes.

- [X] T005 Rename `src/mavesp8266.h` → `src/bridge.h` and `src/mavesp8266.cpp` → `src/bridge.cpp`: update header guard `MAVESP8266_H` → `BRIDGE_H`; rename class `MavESP8266Bridge` → `Bridge`; rename `MavESP8266World` → `World`; rename `MavESP8266Log` → `Log`; rename `MavESP8266Update` → `OtaUpdate` (per plan.md Increment 0 migration table — `OtaUpdate` chosen to avoid collision with Arduino's `Update` class); rename `MavESP8266UpdateImp` in `main.cpp` → `OtaUpdateImp`; rename `linkStatus` struct → `link_status_t`; rename `MAVESP8266_VERSION_*` → `VERSION_*`; add `bool is_armed` and `uint32_t last_heartbeat_ms` fields to `link_status_t` (used by OTA gate in US1 before `telemetry.h` exists)
- [X] T006 [P] Rename `src/mavesp8266_component.h` → `src/component.h` and `.cpp` → `component.cpp`: update header guard; rename class `MavESP8266Component` → `Component`; update `#include "mavesp8266.h"` → `#include "bridge.h"` and all other old includes
- [X] T007 [P] Rename `src/mavesp8266_gcs.h` → `src/gcs.h` and `.cpp` → `gcs.cpp`: update header guard; rename class `MavESP8266GCS` → `Gcs`; update all includes; ensure `gcs_client_t` struct is declared in `gcs.h` per data-model.md
- [X] T008 [P] Rename `src/mavesp8266_vehicle.h` → `src/vehicle.h` and `.cpp` → `vehicle.cpp`: update header guard; rename class `MavESP8266Vehicle` → `Vehicle`; update all includes
- [X] T009 [P] Rename `src/mavesp8266_parameters.h` → `src/parameters.h` and `.cpp` → `parameters.cpp`: update header guard; rename class `MavESP8266Parameters` → `Parameters`; rename struct `stMavEspParameters` → `parameters_t`; rename `MAVESP_WIFI_MODE_*` → `WIFI_MODE_*`; update all includes
- [X] T010 [P] Rename `src/mavesp8266_httpd.h` → `src/httpd.h` and `.cpp` → `httpd.cpp`: update header guard; rename class `MavESP8266Httpd` → `Httpd`; do NOT add `ota_state_t` here — it is declared in `src/ota.h` (created in T016); update all includes
- [X] T011 [P] Rename `src/mavesp8266_ppm.h` → `src/ppm.h` and `.cpp` → `ppm.cpp`: update header guard; rename class `MavESP8266PPM` → `Ppm`; update all includes
- [X] T012 Update `src/main.cpp`: replace all 7 `#include "mavesp8266*.h"` directives with renamed equivalents (`bridge.h`, `component.h`, `gcs.h`, `vehicle.h`, `httpd.h`, `parameters.h`, `ppm.h`); rename `MavESP8266UpdateImp` → `OtaUpdateImp`; update all class name references throughout the file
- [X] T013 Update `src/bridge.h` forward declarations: replace `class MavESP8266Parameters` etc. with `class Parameters`, `class Component`, `class Vehicle`, `class Gcs`, `class Ppm`; verify all cross-include references in bridge.h, component.h, gcs.h, vehicle.h are consistent after renames
- [X] T014 Build verification: `pio run -e esp32s3-cam` MUST produce 0 errors, 0 warnings; resolve any warnings before proceeding
- [ ] T015 Physical flash + smoke test: `pio run -e esp32s3-cam --target upload`; verify device boots, WiFi AP appears, MAVLink forwarding works with a GCS client; additionally set `WIFI_MODE=1` (STA) with an SSID that does not exist, reboot — confirm device falls back to AP mode within 60 s (STA→AP fallback behaviour per plan.md Increment 1)

**Checkpoint**: All source files renamed, build passes clean, device operational.

---

## Phase 3: User Story 1 — MAVLink WiFi Bridge + OTA (Priority: P1) 🎯 MVP

**Goal**: First deployable build with OTA update capability and armed-state safety
gate. After this increment, all further work is deployed wirelessly.

**Independent Test**: MAVLink simulator + Mission Planner; verify bidirectional
forwarding and OTA upload from browser. OTA armed gate verified by simulating
armed HEARTBEAT.

- [ ] T016 [P] [US1] Create `src/ota.h`: declare `ota_state_t` enum (OTA_IDLE, OTA_IN_PROGRESS, OTA_COMPLETE, OTA_ERROR) — this is its single canonical definition; declare `otaBegin()` → `bool`, `otaWrite(uint8_t* data, size_t len)` → `size_t`, `otaEnd()` → `bool`, `otaGetState()` → `ota_state_t`; `src/httpd.h` must `#include "ota.h"` to use the enum
- [ ] T017 [US1] Implement `src/ota.cpp`: `otaBegin()` reads `getWorld()->getVehicle()->getLinkStatus().is_armed` — returns false and sets state to OTA_ERROR if true; else calls `Update.begin(UPDATE_SIZE_UNKNOWN)`, sets state OTA_IN_PROGRESS; `otaWrite()` calls `Update.write()`; `otaEnd()` calls `Update.end()` and sets state OTA_COMPLETE on success — do NOT call `esp_ota_mark_app_valid_cancel_rollback()` here; that call belongs exclusively in T023 (boot confirmation after the new firmware has successfully rebooted) (depends on T016)
- [ ] T018 [P] [US1] Update `src/vehicle.cpp` `readMessage()`: when `msgid == MAVLINK_MSG_ID_HEARTBEAT`, unpack with `mavlink_msg_heartbeat_decode()`, set `_link_status.is_armed = (hb.base_mode & MAV_MODE_FLAG_SAFETY_ARMED) != 0`, set `_link_status.last_heartbeat_ms = millis()`
- [ ] T019 [US1] Update `src/httpd.cpp` GET `/update` handler: call `getWorld()->getVehicle()->getLinkStatus().is_armed`; if true return HTTP 409 with HTML error page "Disarm vehicle before updating firmware"; else return OTA upload form HTML (depends on T017, T018)
- [ ] T020 [US1] Update `src/httpd.cpp` POST `/upload` handler: call `otaBegin()` first; if it returns false propagate HTTP 409; else stream upload chunks via `otaWrite()`; call `otaEnd()` on last chunk; reboot on OTA_COMPLETE (depends on T017)
- [ ] T021 [P] [US1] Update `src/httpd.cpp` GET `/getstatus` JSON response: add `"armed": <bool>` field from `link_status_t.is_armed`; add `"uptimeMs": millis()` field
- [ ] T022 [P] [US1] Update `src/parameters.cpp`: register the 5 new parameters from data-model.md: `CAM_RESOLUTION` (UINT8, default 6), `CAM_QUALITY` (UINT8, default 10), `PWM_ENABLED` (UINT8, default 0), `PWM_SERVO_CHAN` (UINT8, default 5), `PWM_GPIO` (UINT8, default 14)
- [ ] T023 [US1] Implement boot confirmation in `src/main.cpp`: after WiFi connected and first HTTP request is served (use a `bool _ota_confirmed` flag set in the HTTP server's first-request callback), call `esp_ota_mark_app_valid_cancel_rollback()` — prevents rollback on next power cycle; must fire within 30 s of boot or device rolls back (depends on T017)
- [ ] T024 [US1] Build + initial physical flash: `pio run -e esp32s3-cam --target upload`; HIL test: (a) OTA upload succeeds from browser while disarmed → reboots → firmware persists; (b) OTA upload returns 409 while simulated HEARTBEAT shows ARMED; (c) SC-001 timing: start timer when GCS connects to WiFi AP; confirm first HEARTBEAT appears in Mission Planner within 5 s; (d) MAVLink forwarding confirmed bidirectionally in Mission Planner; (e) `/getstatus` returns `"armed": false`

**Checkpoint**: Device is in the field deployable. All future changes via OTA.

---

## Phase 4: User Story 2 — Live Camera Stream (Priority: P2)

**Goal**: Live video stream accessible from QGroundControl (RTSP) and any browser
(HTTP MJPEG). Both sinks use the same OV2640 JPEG frames — no re-encoding.

**Independent Test**: QGC RTSP stream at `rtsp://<ip>:554/mjpeg/1` displays live
video; browser MJPEG at `http://<ip>/stream` displays live video; MAVLink latency
< 10 ms measured while both sinks are active simultaneously.

- [ ] T025 [US2] Add library deps to `platformio.ini` `[env:esp32s3-cam]` `lib_deps`: `ESP Async WebServer` (pinned version from T003) and `Micro-RTSP` (pinned version from T004)
- [ ] T026 [P] [US2] Create `src/camera.h`: declare `camera_config_t` struct (per data-model.md: framesize, quality, enabled); declare `cameraInit(camera_config_t cfg)` → `bool`; declare `cameraStreamTask(void* arg)`; declare `cameraRegisterHttpSink(AsyncResponseStream* stream)` and `cameraUnregisterHttpSink()`
- [ ] T027 [P] [US2] Verify OV2640 camera pin constants in `variants/esp32s3-cam/pins_arduino.h`: identify `PWDN_GPIO_NUM`, `RESET_GPIO_NUM`, `XCLK_GPIO_NUM`, `SIOD_GPIO_NUM`, `SIOC_GPIO_NUM`, `Y9_GPIO_NUM`–`Y2_GPIO_NUM`, `VSYNC_GPIO_NUM`, `HREF_GPIO_NUM`, `PCLK_GPIO_NUM`; document any missing pin defines that need to be added
- [ ] T028 [US2] Implement `src/camera.cpp` `cameraInit()`: populate `esp_camera_config_t` using pin constants from `variants/esp32s3-cam/pins_arduino.h` and `camera_config_t` framesize/quality params; set `config.fb_location = CAMERA_FB_IN_PSRAM` and `config.fb_count = 2` so `esp_camera` owns and pools the PSRAM frame buffers (no manual `heap_caps_malloc` in the stream loop); call `esp_camera_init()`; call `esp_camera_sensor_get()` to apply quality setting; return false if init fails (camera absent); set `camera_config_t.enabled = false` on failure (depends on T026, T027)
- [ ] T029 [US2] Implement `src/camera.cpp` `cameraStreamTask()`: FreeRTOS task, Core 0, priority 3; loop: call `esp_camera_fb_get()` — this returns a PSRAM-backed frame buffer from the pool initialised in T028 (`CAMERA_FB_IN_PSRAM`), no additional `heap_caps_malloc` call; if RTSP session active push frame to `CStreamer`; if HTTP sink registered write frame to `AsyncResponseStream`; call `esp_camera_fb_return()` to return the buffer to the pool; `taskYIELD()` to prevent starving Core 0; exit cleanly when task deleted (depends on T028)
- [ ] T030 [US2] Implement `src/camera.cpp` RTSP server: create `CStreamer` and `WiFiServer` on port 554; `startRtspServer()` function accepts one `CRtspSession` client; integrate session tick into `cameraStreamTask` loop (depends on T029)
- [ ] T031 [US2] Migrate `src/httpd.cpp` from `WebServer` to `AsyncWebServer`: replace `WebServer _webServer` instance with `AsyncWebServer _webServer(80)`; rewrite **all** existing GET handlers — `/`, `/status`, `/getstatus`, `/getParameters`, `/setParameters`, and `/update` — as `_webServer.on(path, HTTP_GET, [](AsyncWebServerRequest* request){...})`; `GET /update` must preserve the armed-gate 409 logic from T019; remove all `_webServer.handleClient()` calls from the update loop
- [ ] T032 [US2] Implement POST `/upload` in `src/httpd.cpp` with `AsyncWebServer` file upload: use `_webServer.on("/upload", HTTP_POST, [](AsyncWebServerRequest* r){}, [](AsyncWebServerRequest* r, String filename, size_t index, uint8_t* data, size_t len, bool final){...})`; call `otaBegin()` on first chunk (index==0), `otaWrite()` per chunk, `otaEnd()` + schedule reboot on final; propagate 409 from `otaBegin()` (depends on T031)
- [ ] T033 [US2] Add GET `/stream` MJPEG endpoint in `src/httpd.cpp`: create `AsyncWebServerResponse` with `multipart/x-mixed-replace; boundary=frame`; call `cameraRegisterHttpSink(stream)`; on client disconnect call `cameraUnregisterHttpSink()`; return 503 with `"Camera not available"` if `camera_config_t.enabled == false` (depends on T026, T031)
- [ ] T034 [US2] Update `src/main.cpp`: include `camera.h`; after WiFi connected call `cameraInit()` with params from `Parameters`; if camera init succeeds, `xTaskCreatePinnedToCore(cameraStreamTask, "cam", 4096, NULL, 3, NULL, 0)` on Core 0; start RTSP server (depends on T028, T030)
- [ ] T035 [US2] Build + OTA push Increment 2: `pio run -e esp32s3-cam`; upload via OTA from web interface
- [ ] T036 [US2] HIL test: (a) QGC shows live video at `rtsp://<ip>:554/mjpeg/1`; (b) browser shows live MJPEG at `/stream` on Chrome/Safari/Firefox; (c) MAVLink latency < 10 ms with both sinks active (Wireshark); (d) OTA upload still works after AsyncWebServer migration; (e) reconnect RTSP 5× — no heap growth (check `/getstatus` `freeHeap`)

**Checkpoint**: Camera streaming live; QGC and browser both working; OTA still functional.

---

## Phase 5: User Story 3 — Web Telemetry Dashboard (Priority: P3)

**Goal**: Full web dashboard with live telemetry, parameter management, and OTA
update — all accessible from any browser without a GCS application.

**Independent Test**: Browser on the WiFi network; verify all telemetry fields
update in real time; change a parameter, reboot, confirm it persisted; upload
firmware via OTA page; submit invalid parameter value and confirm error message.

- [ ] T037 [P] [US3] Create `src/telemetry.h`: declare `telemetry_state_t` struct exactly as specified in `data-model.md`; declare `void telemetryInit()`; declare `void telemetryUpdate(mavlink_message_t* msg)`; declare `const char* telemetryGetJson()`; declare `bool telemetryIsArmed()` accessor
- [ ] T038 [US3] Implement `src/telemetry.cpp` `telemetryUpdate()`: switch on `msg->msgid`; case `MAVLINK_MSG_ID_HEARTBEAT`: decode, set `is_armed`, `base_mode`, `custom_mode`, `system_status`, `system_id`, `last_heartbeat_ms = millis()`; case `MAVLINK_MSG_ID_SYS_STATUS`: decode, set `battery_voltage_mv`, `battery_current_ca`, `battery_remaining`; case `MAVLINK_MSG_ID_GPS_RAW_INT`: decode, set `gps_fix_type`, `gps_satellites`, `lat`, `lon`, `alt_mm`; case `MAVLINK_MSG_ID_VFR_HUD`: decode, set `heading_deg`, `groundspeed_ms`, `airspeed_ms`, `throttle_pct` (depends on T037)
- [ ] T039 [US3] Implement `src/telemetry.cpp` `telemetryGetJson()`: write to static `char _json_buf[512]` (sizing: worst-case ~420 bytes for all fields including lat/lon int32 at max digits and the `link_quality` sub-object; 512 provides ~20% headroom; declare as `constexpr size_t TELEMETRY_JSON_BUF_SIZE = 512` with a comment that this must be increased if new telemetry fields are added); format JSON matching the `telemetry_state_t` fields in `data-model.md` (field names: `armed`, `battery_voltage_mv`, `battery_remaining_pct`, `gps_fix_type`, `gps_satellites`, `lat`, `lon`, `alt_mm`, `heading_deg`, `groundspeed_ms`, `system_status`, `link_quality` sub-object); no heap allocation; return pointer to `_json_buf` (depends on T038)
- [ ] T040 [US3] Update `src/vehicle.cpp` and `src/gcs.cpp`: `#include "telemetry.h"`; call `telemetryUpdate(&msg)` for every parsed MAVLink message; replace direct `_link_status.is_armed` reads in `httpd.cpp` with `telemetryIsArmed()` (depends on T038)
- [ ] T041 [US3] Update `src/httpd.cpp` GET `/getstatus` handler: extend JSON response to include all `telemetry_state_t` fields (`armed`, `battery_voltage_mv`, `battery_remaining_pct`, `gps_fix_type`, `gps_satellites`, `lat`, `lon`, `alt_mm`, `heading_deg`, `groundspeed_ms`, `system_status`) in addition to existing packet counter fields; use `telemetryIsArmed()` instead of `link_status_t.is_armed` (depends on T039, T040)
- [ ] T042 [P] [US3] Create `data/index.html`: live telemetry dashboard served from LittleFS; `fetch("/getstatus", ...)` every 1 s; display armed (green) / disarmed (grey) / unknown (red); battery % with colour coding: > 50% = green, 20–50% = amber, < 20% = red; GPS fix: 3D/RTK = green, 2D = amber, no fix = red; MAVLink link quality: packets_received counter; uptime; loading spinner shown if fetch takes > 500 ms (per FR-012, FR-013); navigation links to `/params`, `/update`, `/stream`
- [ ] T043 [P] [US3] Create `data/params.html`: fetch `/getParameters`; render editable table with one row per parameter; client-side type and range validation before form submit (JS validates `UART_BAUDRATE` ∈ {9600, 57600, 115200, 921600}; `WIFI_CHANNEL` 1–13; `CAM_QUALITY` 0–63; `CAM_RESOLUTION` 0–13; etc.); on validation failure show plain-English error inline, do not POST; on success POST to `/setParameters`; confirm saved message
- [ ] T044 [US3] Update `src/httpd.cpp`: add `LittleFS.begin()` in `Httpd::begin()`; add `GET /status` → serve `data/index.html` from LittleFS (`request->send(LittleFS, "/index.html", "text/html")`); add `GET /params` → serve `data/params.html`; add catch-all static file handler for `data/` assets with correct MIME types; implement `POST /uploadfs` LittleFS OTA endpoint — same upload handler pattern as `POST /upload` but calls `Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS)` instead of `U_FLASH`; armed check NOT required for filesystem uploads; on success call `LittleFS.end()` and return 200 OK (no reboot); armed check still applies to `POST /upload` firmware only (depends on T042, T043)
- [ ] T045 [US3] Build firmware + OTA push + upload LittleFS assets wirelessly: `pio run -e esp32s3-cam`; push firmware binary via `POST /upload` from browser OTA page; then upload LittleFS image by POSTing the `.littlefs.bin` artifact (from `.pio/build/esp32s3-cam/*.littlefs.bin`) to `POST /uploadfs` — no USB access required after Phase 3; verify `GET /status` serves updated HTML immediately after upload completes
- [ ] T046 [US3] HIL test: (a) dashboard armed/disarmed/battery/GPS update in real time; (b) change `WIFI_CHANNEL` via params page → reboot → confirm persisted in `/getParameters`; (c) enter `WIFI_CHANNEL=99` → plain-English error, no POST; (d) firmware OTA upload via `/update` page succeeds; (e) dashboard loads in < 2 s on WiFi

**Checkpoint**: Full web interface operational. Device manageable entirely without USB.

---

## Phase 6: User Story 4 — Motor Driver & Signal Processing (Priority: P4)

**Goal**: PPM/PWM motor output from `SERVO_OUTPUT_RAW` MAVLink channel with
failsafe. Feature is gated by `PWM_ENABLED` parameter so it is safe when unused.

**Independent Test**: MAVLink simulator sending `SERVO_OUTPUT_RAW` channel 5 at
various values; oscilloscope/logic analyser on GPIO14 confirms 490 Hz ± 10 Hz
and correct duty cycle; disconnect UART for > 1 s, confirm PWM drops to 0%.

- [ ] T047 [P] [US4] Update `src/ppm.h`: declare `Ppm::begin(uint8_t gpio, uint8_t channel, uint8_t servo_chan)` with LEDC channel 0, frequency 490 Hz, resolution 8-bit; declare `Ppm::setDuty(uint16_t servo_us)` mapping 1000–2000 µs → 0–255 duty; declare `Ppm::failsafe()` sets duty to 0; add `uint32_t _last_output_ms` private member for watchdog
- [ ] T048 [US4] Update `src/ppm.cpp` `begin()`: check `PWM_ENABLED` parameter — if 0, return immediately without calling `ledcSetup()` or `ledcAttachPin()`; otherwise init LEDC on GPIO14 (from `PWM_GPIO` parameter), channel 0, 490 Hz, 8-bit; note GPIO14 and 490 Hz are documented in `MOTOR_DRIVER.md` and must come from `Parameters` not hardcoded (depends on T047)
- [ ] T049 [US4] Update `src/ppm.cpp`: implement failsafe watchdog — in `Ppm::handleMessage()` for `MAVLINK_MSG_ID_SERVO_OUTPUT_RAW`: extract channel `PWM_SERVO_CHAN` value, call `setDuty()`, set `_last_output_ms = millis()`; add `Ppm::update()` called from main loop: if `millis() - _last_output_ms > 1000` and PWM active, call `failsafe()` (depends on T048)
- [ ] T050 [US4] Update `src/vehicle.cpp`: forward `MAVLINK_MSG_ID_SERVO_OUTPUT_RAW` messages to `getWorld()->getPpm()->handleMessage(&msg)` (depends on T049)
- [ ] T051 [US4] Build + OTA push Increment 4; HIL test (SC-006 full range): (a) channel 5 at 1000 µs → duty 0% ± 2% at 490 Hz ± 10 Hz; (b) channel 5 at 1500 µs → duty 50% ± 2% at 490 Hz ± 10 Hz; (c) channel 5 at 2000 µs → duty 100% ± 2% at 490 Hz ± 10 Hz; (d) SC-007: UART disconnected > 1 s → duty falls to 0% within 1.1 s; (e) `PWM_ENABLED = 0` → no LEDC init, GPIO14 remains hi-z; (f) MAVLink forwarding and camera stream unaffected while PWM active

**Checkpoint**: Motor driver functional; failsafe verified; MAVLink bridge unaffected.

---

## Phase 8: Polish & Cross-Cutting Concerns

**Purpose**: Edge cases, performance validation, documentation cleanup.

- [ ] T060 [P] Update `specs/001-esp32-companion-computer/quickstart.md`: fix stale serial output example "MavBridge starting..." → "Bridge starting..." (or match whatever log prefix the renamed class actually emits); verify all other class name references in quickstart.md match renamed names
- [ ] T061 [P] Add free heap monitoring in `src/main.cpp`: in main `loop()`, every 60 s sample `ESP.getFreeHeap()` → store in `link_status_t.free_heap_bytes` → log to `Serial`; `/getstatus` already returns `freeHeap` — verify it reads from `link_status_t.free_heap_bytes`
- [ ] T062 Verify edge cases from spec.md: (a) autopilot UART unplugged after boot — WiFi AP remains active, web interface loads, `/getstatus` shows `vehicleConnected: false`; (b) OTA upload interrupted mid-transfer — device does NOT reboot, returns FAIL, remains operational; (c) camera module physically absent — web interface loads normally, `/stream` returns 503, no crash
- [ ] T063 Full performance validation with all features active (US1–US4): measure MAVLink end-to-end latency with Wireshark (must be < 10 ms); measure free heap with `/getstatus` (must be > 20 KB); measure firmware binary size from PlatformIO build output (must be < 1.5 MB); document pass/fail results in `quickstart.md` Performance section

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 (Setup)**: No dependencies — start immediately
- **Phase 2 (Foundation)**: Depends on Phase 1 completion — **BLOCKS all user stories**
- **Phase 3 (US1)**: Depends on Phase 2 — first deployable build; OTA enables remote deploy from here
- **Phase 4 (US2)**: Depends on Phase 3 (OTA must work for deployment; AsyncWebServer replaces existing WebServer)
- **Phase 5 (US3)**: Depends on Phase 4 (needs AsyncWebServer from US2)
- **Phase 6 (US4)**: Depends on Phase 3 — can run in parallel with Phase 4 and 5 if capacity allows
- **Phase 8 (Polish)**: Depends on all desired phases complete

### User Story Dependencies

```
Foundation (Phase 2)
  └── US1: MAVLink Bridge + OTA (Phase 3) ← first physical flash required here
        └── US2: Camera Streaming (Phase 4)
        └── US3: Web Dashboard (Phase 5)
        └── US4: Motor Driver (Phase 6) ← independent of US2/US3
```

### Parallel Opportunities Per Phase

**Phase 1**: T001–T004 all parallel after the build check
**Phase 2**: T005 first; then T006, T007, T008, T009, T010, T011 all [P]; T012, T013 after those; T014 gates everything
**Phase 3**: T016, T018, T021, T022 all [P] before T017; T017 then T019, T020 sequential
**Phase 4**: T026, T027 [P] before T028; T025 can go any time; T028–T030 sequential; T031–T033 sequential on httpd.cpp
**Phase 5**: T037 first; T038, T039 sequential; T042, T043 [P]; T040, T041, T044 sequential
**Phase 6**: T047, T048 [P]; T049, T050 sequential; T051 last
**Phase 8**: T060, T061, T062, T063 in order (T063 depends on T062)

---

## Implementation Strategy

**MVP scope**: Phases 1–3 (Foundation + US1). After T024 the device is deployable
and all remaining increments are delivered wirelessly. Suggested per-session goals:

| Session | Phases | Outcome |
|---|---|---|
| 1 | 1 + 2 | Renamed codebase, clean build, device unchanged |
| 2 | 3 | OTA armed gate live — remote deployment enabled |
| 3 | 4 | Camera streaming to QGC + browser |
| 4 | 5 | Full web dashboard + parameter management |
| 5 | 6 + 8 | Motor driver with failsafe + polish |
