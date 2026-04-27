/****************************************************************************
 *
 * Copyright (c) 2015, 2016 Gus Grubba. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file ota.cpp
 * OTA update state machine implementation
 *
 * Armed-state safety gate: otaBegin() reads Vehicle link_status_t.is_armed.
 * If the vehicle is armed the update is refused (returns false, state=OTA_ERROR).
 * The only call to esp_ota_mark_app_valid_cancel_rollback() is in main.cpp (T023),
 * not here — it fires after the new firmware has booted and served its first HTTP
 * request, confirming the firmware is valid.
 */

#include "ota.h"
#include "bridge.h"
#include "vehicle.h"

#ifdef ARDUINO_ARCH_ESP32
    #include <Update.h>
#else
    #include <ESP8266HTTPUpdateServer.h>
#endif

static ota_state_t _state = OTA_IDLE;

//---------------------------------------------------------------------------------
// otaBegin — armed-state gate + Update initialisation
// Returns false (state=OTA_ERROR) if vehicle is armed or Update.begin() fails.
bool otaBegin()
{
    // Armed-state safety gate
    link_status_t* vs = getWorld()->getVehicle()->getStatus();
    if (vs->is_armed) {
        _state = OTA_ERROR;
        return false;
    }

#ifdef ARDUINO_ARCH_ESP32
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        _state = OTA_ERROR;
        return false;
    }
#else
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin(maxSketchSpace)) {
        _state = OTA_ERROR;
        return false;
    }
#endif

    _state = OTA_IN_PROGRESS;
    return true;
}

//---------------------------------------------------------------------------------
// otaWrite — stream a chunk of the firmware binary into the Update engine.
size_t otaWrite(uint8_t* data, size_t len)
{
    if (_state != OTA_IN_PROGRESS) {
        return 0;
    }
    return Update.write(data, len);
}

//---------------------------------------------------------------------------------
// otaEnd — finalise the update.  Does NOT call
// esp_ota_mark_app_valid_cancel_rollback() — that belongs in main.cpp T023.
bool otaEnd()
{
    if (_state != OTA_IN_PROGRESS) {
        return false;
    }
    if (Update.end(true)) {
        _state = OTA_COMPLETE;
        return true;
    }
    _state = OTA_ERROR;
    return false;
}

//---------------------------------------------------------------------------------
ota_state_t otaGetState()
{
    return _state;
}
