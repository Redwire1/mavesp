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
 * @file ppm.h
 * ESP8266 Wifi AP, MavLink UART/UDP Bridge
 *
 * @author Gus Grubba <mavlink@grubba.com>
 */

#ifndef PPM_H
#define PPM_H

#include <Arduino.h>
#include <stdint.h>

//-- PWM output pin (GPIO 14)
#define PWM_OUTPUT_PIN      14
#define PWM_FREQ            490
#define PWM_RESOLUTION      8
#define PWM_CHANNEL         0

//-- Servo pulse range (microseconds)
#define SERVO_MIN_PULSE     1000
#define SERVO_MAX_PULSE     2000

//-- Servo timeout (ms without MAVLink servo output → stop PWM)
#define SERVO_TIMEOUT_MS    1000

#undef F
#include <ardupilotmega/mavlink.h>

class Ppm {
public:
    Ppm();

    void    begin               ();
    void    update              ();
    void    handleServoOutput   (const mavlink_message_t* message);

    uint16_t    getServoValue   ();
    float       getDutyCycle    ();
    uint8_t     getServoChannel ();
    bool        isSignalValid   ();

private:
    void        _updatePWMOutput    ();
    void        _setFailsafe        ();
    uint16_t    _mapServoValueToDuty(uint16_t servoValue);

private:
    uint8_t     _servoChannel;
    uint16_t    _servoValue;
    float       _dutyCycle;
    bool        _signalValid;
    bool        _initialized;
    unsigned long _lastServoUpdate;
};

#endif
