# HTTP API Contract: ESP32-S3 Companion Computer

**Branch**: `001-esp32-companion-computer`
**Date**: 2026-04-21
**Base URL**: `http://<device-ip>/` (port 80)
**Protocol**: HTTP/1.1

All endpoints are served by the companion computer directly. No external server
is involved. Endpoints introduced in each increment are noted.

---

## Authentication

### Public endpoints (no auth required)
All endpoints EXCEPT those under `/api/v1/` are public. They are accessible to
any device on the WiFi network.

### LLM API endpoints (X-Api-Key required)
Endpoints under `/api/v1/` require the header:
```
X-Api-Key: <configured-secret>
```
Requests missing this header or with an incorrect value receive:
```
HTTP 401 Unauthorized
Content-Type: text/plain

Unauthorized
```
The rejection event is logged to `link_status_t` but NOT broadcast via
MAVLink.

---

## Web Interface Endpoints (Increment 1–3)

### GET /
**Increment**: 1
**Description**: Root page. Redirects to `/status` or renders a simple
landing page with links to status, parameters, update, and camera.
```
Response: 200 OK | 302 Found → /status
Content-Type: text/html
```

### GET /status
**Increment**: 3 (stub in Increment 1)
**Description**: Web dashboard page. Returns HTML with live telemetry summary,
link quality, free heap, and uptime. In Increment 1 returns a plain-text status
stub.
```
Response: 200 OK
Content-Type: text/html
```

### GET /getstatus
**Increment**: 1 (existing endpoint, retained)
**Description**: Returns MAVLink bridge statistics as JSON.
```
Response: 200 OK
Content-Type: application/json

{
  "vehicleConnected": true,
  "gcsConnected": true,
  "vehiclePacketsReceived": 12340,
  "vehiclePacketsSent": 12330,
  "vehicleParseErrors": 0,
  "gcsPacketsReceived": 450,
  "gcsPacketsSent": 448,
  "gcsParseErrors": 0,
  "freeHeap": 48320,
  "uptimeMs": 86400000
}
```

### GET /getParameters
**Increment**: 1 (existing endpoint, retained)
**Description**: Returns all system parameters as JSON array.
```
Response: 200 OK
Content-Type: application/json

[
  { "id": "UART_BAUDRATE", "value": 921600, "type": 6, "index": 0, "readOnly": false },
  { "id": "WIFI_MODE",     "value": 0,      "type": 1, "index": 1, "readOnly": false },
  ...
]
```

### GET /setParameters
**Increment**: 1 (existing endpoint, retained)
**Description**: Sets one or more parameters via URL query string.
Out-of-range values return 400.
```
GET /setParameters?UART_BAUDRATE=115200&WIFI_CHANNEL=6

Response: 200 OK
Content-Type: text/plain
OK
```
```
Response (invalid value): 400 Bad Request
Content-Type: text/plain
BAD ARGS
```

### GET /update
**Increment**: 1 (existing endpoint, improved UI)
**Description**: Returns OTA firmware update page (HTML form). If vehicle is
currently ARMED, returns an error page instead.
```
Response (disarmed): 200 OK
Content-Type: text/html
[OTA upload form]
```
```
Response (armed): 409 Conflict
Content-Type: text/html
[Error: Disarm vehicle before updating firmware]
```

### POST /upload
**Increment**: 1 (existing endpoint, safety gate added)
**Description**: Receives binary firmware upload (multipart/form-data).
Writes to inactive OTA partition. Reboots on success.
```
Request:
Content-Type: multipart/form-data; boundary=...
[binary firmware .bin file]

Response (success): 200 OK
Content-Type: text/plain
OK
[device reboots]
```
```
Response (update error): 200 OK
Content-Type: text/plain
FAIL
```
```
Response (armed): 409 Conflict
Content-Type: text/plain
Disarm vehicle before updating firmware
```

---

## Camera Streaming (Increment 2)

The companion computer exposes two simultaneous camera sinks from the same JPEG
frame pipeline:

| Sink | Protocol | Address | Consumer |
|---|---|---|---|
| RTSP server | RTSP/RTP MJPEG | `rtsp://<device-ip>:554/mjpeg/1` | QGroundControl, VLC, any RTSP client |
| HTTP endpoint | multipart MJPEG | `http://<device-ip>/stream` | Browser direct access |

### RTSP Video Stream (QGroundControl)
**Protocol**: RTSP 1.0 on TCP port 554, MJPEG-over-RTP
**QGC setup**: Settings → Video → Source: `RTSP Video Stream` →
URL: `rtsp://<device-ip>:554/mjpeg/1`

This is the correct format for QGroundControl. QGC uses GStreamer internally
and will decode MJPEG-over-RTP automatically. No additional configuration is
required.

### GET /stream
**Increment**: 2
**Description**: MJPEG live stream for direct browser viewing. Returns a
multipart HTTP response that remains open, pushing JPEG frames continuously.

```
Response: 200 OK
Content-Type: multipart/x-mixed-replace; boundary=frame

--frame
Content-Type: image/jpeg
Content-Length: <bytes>

<JPEG data>
--frame
...
```

Both the RTSP server and the HTTP `/stream` endpoint consume the same JPEG
frames from the camera task — no re-encoding cost.

**If camera module absent at boot**:
```
Response: 503 Service Unavailable
Content-Type: text/plain
Camera not available
```

---

## LLM Companion API (Increment 5)

All endpoints under `/api/v1/` require `X-Api-Key` header (see Authentication).
These endpoints are only active when `LLM_API_ENABLED = 1`.

If `LLM_API_ENABLED = 0`:
```
Response: 404 Not Found
```

### GET /api/v1/telemetry
**Description**: Returns current vehicle telemetry state as JSON.

The JSON fields correspond to members of `telemetry_state_t` and
`link_status_t`.

```
Request:
GET /api/v1/telemetry HTTP/1.1
X-Api-Key: <secret>
```
```
Response: 200 OK
Content-Type: application/json

{
  "armed": false,
  "battery_voltage_mv": 12400,
  "battery_remaining_pct": 72,
  "gps_fix_type": 3,
  "gps_satellites": 9,
  "lat": -337654321,
  "lon": 1510123456,
  "alt_mm": 52000,
  "heading_deg": 245.3,
  "groundspeed_ms": 0.0,
  "system_status": 3,
  "link_quality": {
    "packets_received": 12340,
    "parse_errors": 0,
    "uptime_ms": 86400000
  }
}
```

### POST /api/v1/command
**Description**: Sends an allowlisted command to the autopilot via MAVLink.

```
Request:
POST /api/v1/command HTTP/1.1
X-Api-Key: <secret>
Content-Type: application/json

{
  "type": "SET_PARAMETER",
  "param_id": "UART_BAUDRATE",
  "param_value": 115200
}
```
```
Response (success): 202 Accepted
Content-Type: application/json

{
  "status": "dispatched",
  "type": "SET_PARAMETER",
  "param_id": "UART_BAUDRATE"
}
```
```
Response (not on allowlist): 400 Bad Request
Content-Type: application/json

{
  "error": "command_not_allowed",
  "message": "Command type 'RAW_MAVLINK' is not permitted"
}
```
```
Response (invalid param): 422 Unprocessable Entity
Content-Type: application/json

{
  "error": "invalid_value",
  "message": "UART_BAUDRATE value 9999 is out of range"
}
```
```
Response (read-only param): 422 Unprocessable Entity
Content-Type: application/json

{
  "error": "read_only",
  "message": "Parameter SYS_STATUS is read-only"
}
```

The request body maps to an `llm_cmd_t` struct after validation.

**Supported command types**:
| `type` value | MAVLink action |
|---|---|
| `"SET_PARAMETER"` | `PARAM_SET` message to autopilot |
| `"REQUEST_MODE"` | `COMMAND_LONG (MAV_CMD_DO_SET_MODE)` to autopilot |

All other `type` values return 400.

### POST /api/v1/event (inbound from MimiClaw — NOT served by this device)
> This endpoint is on the **MimiClaw device**, not this companion computer.
> This companion computer POSTs to `LLM_PUSH_URL` with the payload below when
> a threshold event fires.

**Outbound event push payload**:
```json
POST <LLM_PUSH_URL>
Content-Type: application/json

{
  "event": "BATTERY_LOW",
  "battery_remaining_pct": 18,
  "timestamp_ms": 86412000,
  "message": "Battery at 18% — consider returning to home"
}
```

---

## Error Response Conventions

| Code | Meaning |
|---|---|
| 200 OK | Request succeeded |
| 202 Accepted | Command dispatched to autopilot (not yet ACKed) |
| 302 Found | Redirect |
| 400 Bad Request | Invalid parameter name, value, or command type |
| 401 Unauthorized | Missing or incorrect X-Api-Key |
| 404 Not Found | Endpoint not found or LLM API disabled |
| 409 Conflict | OTA refused (vehicle armed) |
| 422 Unprocessable Entity | Structurally valid but logically invalid request |
| 503 Service Unavailable | Camera absent or bridge not initialised |
