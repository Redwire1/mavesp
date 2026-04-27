"""
PlatformIO custom upload script — HTTP OTA for esp32s3-cam-ota env.

Posts the firmware binary to the device's /upload endpoint using the same
multipart/form-data format accepted by the WebServer handler in httpd.cpp.
The device must be reachable on port 80 at the IP set in `upload_port`.

Usage (VS Code / terminal):
    pio run -e esp32s3-cam-ota --target upload

The armed-state gate in the firmware returns HTTP 409 if the vehicle is armed.
Upload is rejected and the script exits non-zero in that case.
"""

import sys
import urllib.request
import urllib.error

Import("env")  # noqa: F821  (PlatformIO injects this into the script scope)


def upload_ota(source, target, env):
    firmware = str(source[0])
    host = env.GetProjectOption("upload_port")
    url = f"http://{host}/upload"

    print(f"\nOTA upload -> {url}")

    boundary = "----PlatformIOOTABoundary"
    with open(firmware, "rb") as f:
        firmware_bytes = f.read()

    body = (
        f"--{boundary}\r\n"
        f'Content-Disposition: form-data; name="update"; filename="firmware.bin"\r\n'
        f"Content-Type: application/octet-stream\r\n\r\n"
    ).encode() + firmware_bytes + f"\r\n--{boundary}--\r\n".encode()

    req = urllib.request.Request(
        url,
        data=body,
        headers={"Content-Type": f"multipart/form-data; boundary={boundary}"},
        method="POST",
    )

    try:
        with urllib.request.urlopen(req, timeout=120) as resp:
            print(f"OTA complete (HTTP {resp.status}) — device is rebooting")
    except urllib.error.HTTPError as e:
        if e.code == 409:
            print("OTA REJECTED: vehicle is armed — disarm before uploading firmware")
        else:
            print(f"OTA failed: HTTP {e.code}")
        sys.exit(1)
    except OSError as e:
        print(f"OTA failed: {e}")
        print(f"Check that the device is reachable at {host} on port 80")
        sys.exit(1)


env.Replace(UPLOADCMD=upload_ota)
