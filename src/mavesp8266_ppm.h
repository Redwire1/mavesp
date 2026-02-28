/****************************************************************************
 *
 * Copyright (c) 2025. All rights reserved.
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
 * @file mavesp8266_ppm.h
 * ArduPilot Servo Output to Motor Controller PWM Converter
 * Extracts servo output from MAVLink SERVO_OUTPUT_RAW messages and outputs 16kHz PWM
 *
 */

#ifndef MAVESP8266_PPM_H
#define MAVESP8266_PPM_H

#include <Arduino.h>
#include "../lib/mavlink/mavlink_types.h"
#include "../lib/mavlink/common/common.h"

// GPIO Pin Configuration
#define PWM_OUTPUT_PIN  14   // GPIO14 - 16kHz PWM output to motor controller

// PWM Output Configuration (490Hz for ZS-X11H motor controller)
#define PWM_FREQ        490     // 490Hz output frequency (ZS-X11H requirement)
#define PWM_RESOLUTION  8       // 8-bit resolution (0-255)
#define PWM_CHANNEL     0       // LEDC channel 0

// Servo Input Configuration
#define SERVO_MIN_PULSE     1000    // Minimum valid servo value (us)
#define SERVO_MAX_PULSE     2000    // Maximum valid servo value (us)
#define SERVO_TIMEOUT_MS    1000    // Signal loss timeout (ms) - SERVO_OUTPUT_RAW at ~50Hz

class MavESP8266PPM
{
public:
    MavESP8266PPM();
    ~MavESP8266PPM();
    
    void    begin();
    void    update();
    
    // MAVLink message handler
    void    handleServoOutput(const mavlink_message_t* msg);
    
    // Getters for monitoring
    uint16_t getServoValue()    { return _servoValue; }
    uint16_t getDutyCycle()     { return _dutyCycle; }
    uint8_t  getServoChannel()  { return _servoChannel; }
    bool     isSignalValid()    { return _signalValid; }
    
private:
    // Signal processing
    void        _updatePWMOutput();
    void        _setFailsafe();
    uint16_t    _mapServoValueToDuty(uint16_t servoValue);
    
    // State variables
    uint16_t        _servoValue;        // Current servo value (1000-2000us)
    uint16_t        _dutyCycle;         // Current PWM duty cycle (0-4095)
    uint8_t         _servoChannel;      // Servo channel to monitor (5-16)
    unsigned long   _lastServoUpdate;   // Last time SERVO_OUTPUT_RAW received
    bool            _signalValid;       // True if servo signal is recent and valid
    bool            _initialized;       // True after begin() called
};

#endif // MAVESP8266_PPM_H
