# HTTP API Contract: ESP32-S3 Companion Computer

**Branch**: `001-esp32-companion-computer`
**Date**: 2026-04-21
**Base URL**: `http://<device-ip>/` (port 80)
**Protocol**: HTTP/1.1

All endpoints are served by the companion computer directly. No external server
is involved. Endpoints introduced in each increment are noted.

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

### POST /uploadfs
**Increment**: 3
**Description**: Receives a LittleFS filesystem image (`.littlefs.bin`) and
flashes it to the SPIFFS/LittleFS OTA partition using `Update.begin(size, U_SPIFFS)`.
Does NOT require the vehicle to be disarmed (filesystem content is not
safety-critical). Does NOT reboot on success — the updated files are
immediately accessible via LittleFS.
```
Request:
Content-Type: multipart/form-data; boundary=...
[LittleFS .littlefs.bin image]

Response (success): 200 OK
Content-Type: text/plain
OK
```
```
Response (update error): 200 OK
Content-Type: text/plain
FAIL
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

## Error Response Conventions

| Code | Meaning |
|---|---|
| 200 OK | Request succeeded |
| 302 Found | Redirect |
| 400 Bad Request | Invalid parameter name or value |
| 404 Not Found | Endpoint not found |
| 409 Conflict | OTA refused (vehicle armed) |
| 503 Service Unavailable | Camera absent or bridge not initialised |
