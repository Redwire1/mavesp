# Data Model: ESP32-S3 Companion Computer for ArduPilot

**Branch**: `001-esp32-companion-computer`
**Date**: 2026-04-21
**Input**: spec.md, research.md

All structs defined here are declared in their respective headers under `src/`.
All identifiers follow the Naming Conventions in the constitution: `*_t`
struct names, `snake_case` member variables, bare `PascalCase` class names,
no-prefix file names.

---

## parameters_t

**Purpose**: A single named, typed, persistent configuration parameter stored in
NVS. Replaces the legacy `stMavEspParameters` (was `stMavEspParameters` in
`mavesp8266_parameters.h`). The full parameter set is initialised at boot and
read/written via the MAVLink parameter protocol and the web interface.

**Defined in**: `src/parameters.h`

```cpp
struct parameters_t {
    char        id[MAVLINK_MSG_PARAM_VALUE_FIELD_PARAM_ID_LEN]; // MAVLink param ID (16 chars)
    void*       value;      // Pointer to the live value in memory
    uint16_t    index;      // MAVLink parameter index
    uint8_t     length;     // Byte length of the value type
    uint8_t     type;       // MAVLink param type (MAV_PARAM_TYPE_*)
    bool        read_only;  // If true, set requests are rejected
};
```

**Parameters defined** (all persistent in NVS unless noted):

| Parameter ID | Type | Default | Notes |
|---|---|---|---|
| `UART_BAUDRATE` | UINT32 | 921600 | Autopilot UART baud rate |
| `WIFI_MODE` | UINT8 | 0 (AP) | 0 = AP mode, 1 = STA mode |
| `WIFI_SSID` | CHAR[32] | `"ArduPilot"` | WiFi SSID for AP or STA |
| `WIFI_PASSWORD` | CHAR[32] | `"ardupilot"` | WiFi password |
| `WIFI_CHANNEL` | UINT8 | 11 | AP channel (1–13) |
| `WIFI_STA_IP` | UINT32 | 0 | STA static IP (0 = DHCP) |
| `WIFI_STA_GW` | UINT32 | 0 | STA gateway |
| `WIFI_STA_SUBNET` | UINT32 | 0 | STA subnet mask |
| `UDP_HOST_PORT` | UINT16 | 14550 | GCS incoming UDP port |
| `UDP_CLIENT_PORT` | UINT16 | 14555 | GCS outgoing UDP port |
| `CAM_RESOLUTION` | UINT8 | 6 (SVGA) | esp_camera framesize_t value |
| `CAM_QUALITY` | UINT8 | 10 | JPEG quality (0–63, lower = better) |
| `PWM_ENABLED` | UINT8 | 0 | 0 = disabled, 1 = enabled |
| `PWM_SERVO_CHAN` | UINT8 | 5 | SERVO_OUTPUT_RAW channel number |
| `PWM_GPIO` | UINT8 | 14 | GPIO pin for PWM output |
---

## telemetry_state_t

**Purpose**: In-memory snapshot of the current vehicle state derived from
received MAVLink messages. Updated by the MAVLink bridge on every relevant
message. Read by the web interface. Not persisted.

**Defined in**: `src/telemetry.h`

```cpp
struct telemetry_state_t {
    // Derived from HEARTBEAT
    uint8_t     system_id;
    uint8_t     base_mode;
    uint32_t    custom_mode;
    uint8_t     system_status;      // MAV_STATE_*
    bool        is_armed;
    uint32_t    last_heartbeat_ms;

    // Derived from SYS_STATUS
    uint16_t    battery_voltage_mv; // millivolts
    int16_t     battery_current_ca; // centiamperes (-1 = unknown)
    int8_t      battery_remaining;  // percent (-1 = unknown)

    // Derived from GPS_RAW_INT or GLOBAL_POSITION_INT
    uint8_t     gps_fix_type;       // GPS_FIX_TYPE_*
    uint8_t     gps_satellites;
    int32_t     lat;                // degrees × 1e7
    int32_t     lon;                // degrees × 1e7
    int32_t     alt_mm;             // millimetres above MSL

    // Derived from VFR_HUD or GLOBAL_POSITION_INT
    float       heading_deg;
    float       groundspeed_ms;
    float       airspeed_ms;
    float       throttle_pct;
};
```

**State transitions**:
- Initialised to zero / unknown at boot.
- `is_armed` derived from `base_mode & MAV_MODE_FLAG_SAFETY_ARMED`.
- `last_heartbeat_ms` used to detect autopilot link loss (> 10 s = link lost).

---

## link_status_t

**Purpose**: Running packet counters for the MAVLink bridge. Shown in the web
status page.

**Defined in**: `src/bridge.h`

```cpp
struct link_status_t {
    uint32_t    packets_received;   // From vehicle UART
    uint32_t    packets_sent;       // To GCS UDP
    uint32_t    parse_errors;       // MAVLink framing / CRC errors
    uint32_t    gcs_packets_in;     // From GCS UDP
    uint32_t    gcs_packets_out;    // To vehicle UART
    uint32_t    gcs_parse_errors;
    uint32_t    uptime_ms;
    uint32_t    free_heap_bytes;    // Sampled every 60 s

    // ⚠️ TEMPORARY BRIDGE FIELDS (Phase 2–3 only)
    // Added in T005 (Increment 0) to provide armed-state for the OTA gate
    // before telemetry.h exists. Removed in T040 (Increment 3) when
    // telemetryIsArmed() / telemetry_state_t become the authoritative source.
    bool        is_armed;           // Set from HEARTBEAT MAV_MODE_FLAG_SAFETY_ARMED
    uint32_t    last_heartbeat_ms;  // millis() of last HEARTBEAT from vehicle
};
```

**Lifecycle of temporary bridge fields**:
- `is_armed` and `last_heartbeat_ms` are set in `vehicle.cpp` from Phase 2 onward (T018).
- The OTA gate (`httpd.cpp`, T019) and `/getstatus` response (T021) read `is_armed` from this struct in Phases 3–4.
- In Phase 5, T040 calls `telemetryUpdate()` for every MAVLink message and T041 migrates `httpd.cpp` to read from `telemetryIsArmed()` instead. At that point these two fields are dead code and MUST be removed from the struct.

---

## gcs_client_t

**Purpose**: Identifies a GCS UDP client currently communicating with the
companion computer. The bridge currently tracks one client; the struct is sized
to support extension to multiple clients.

**Defined in**: `src/gcs.h`

```cpp
struct gcs_client_t {
    IPAddress   addr;
    uint16_t    port;
    uint32_t    last_seen_ms;
    bool        active;
};
```

---

## camera_config_t

**Purpose**: Runtime camera configuration derived from parameter entries.
Passed to `esp_camera_sensor_get()` after init to adjust quality/resolution
without reboot.

**Defined in**: `src/camera.h`

```cpp
struct camera_config_t {
    uint8_t     framesize;  // esp_camera framesize_t
    uint8_t     quality;    // JPEG quality 0–63
    bool        enabled;    // false if camera module absent at boot
};
```

**Validation rules**:
- `framesize` must be a valid `framesize_t` value (0–23); reject out-of-range.
- `quality` must be 0–63; reject out-of-range.

---

## ota_state_t (enum)

**Purpose**: Tracks the state of an in-progress OTA firmware update.

**Defined in**: `src/ota.h` (included by `httpd.h` via `#include "ota.h"`)

```cpp
enum ota_state_t : uint8_t {
    OTA_IDLE,
    OTA_IN_PROGRESS,
    OTA_COMPLETE,
    OTA_ERROR
};
```

**State transitions**:
```
OTA_IDLE → OTA_IN_PROGRESS  (upload begins, armed check passes)
OTA_IN_PROGRESS → OTA_COMPLETE  (Update.end() succeeds, reboot pending)
OTA_IN_PROGRESS → OTA_ERROR     (write error or armed check fails)
OTA_COMPLETE → (reboot)
OTA_ERROR → OTA_IDLE            (next request resets state)
```
