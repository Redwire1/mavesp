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
 * @file mavesp8266_ppm.cpp
 * ArduPilot Servo Output to Motor Controller PWM Converter
 *
 */

#include "mavesp8266.h"
#include "mavesp8266_ppm.h"
#include "mavesp8266_parameters.h"

//---------------------------------------------------------------------------------
MavESP8266PPM::MavESP8266PPM()
    : _servoValue(1500)
    , _dutyCycle(0)
    , _servoChannel(5)
    , _lastServoUpdate(0)
    , _signalValid(false)
    , _initialized(false)
{
}

//---------------------------------------------------------------------------------
MavESP8266PPM::~MavESP8266PPM()
{
}

//---------------------------------------------------------------------------------
void
MavESP8266PPM::begin()
{
#ifdef ARDUINO_ARCH_ESP32
    // Load servo channel from parameters (5-16, default 5)
    _servoChannel = getWorld()->getParameters()->getPWMServoChannel();
    
    // Validate and clamp to 5-16 range
    if (_servoChannel < 5) {
        _servoChannel = 5;
    } else if (_servoChannel > 16) {
        _servoChannel = 16;
    }
    
    // Configure LEDC for 490Hz PWM output (ZS-X11H requirement)
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PWM_OUTPUT_PIN, PWM_CHANNEL);
    
    // Start with motor off (0% duty cycle)
    ledcWrite(PWM_CHANNEL, 0);
    _dutyCycle = 0;
    
    _initialized = true;
    _lastServoUpdate = millis();
#else
    // PWM Converter only supported on ESP32
#endif
}

//---------------------------------------------------------------------------------
void
MavESP8266PPM::update()
{
    if (!_initialized) {
        return;
    }
    
    // Check for servo signal timeout (1000ms)
    if (_signalValid && (millis() - _lastServoUpdate) > SERVO_TIMEOUT_MS) {
        _signalValid = false;
        _setFailsafe();
    }
}

//---------------------------------------------------------------------------------
void
MavESP8266PPM::handleServoOutput(const mavlink_message_t* msg)
{
    if (!_initialized) {
        return;
    }
    
    // Extract servo value based on configured channel (5-16)
    uint16_t servoValue = 0;
    
    switch (_servoChannel) {
        case 5:  servoValue = mavlink_msg_servo_output_raw_get_servo5_raw(msg); break;
        case 6:  servoValue = mavlink_msg_servo_output_raw_get_servo6_raw(msg); break;
        case 7:  servoValue = mavlink_msg_servo_output_raw_get_servo7_raw(msg); break;
        case 8:  servoValue = mavlink_msg_servo_output_raw_get_servo8_raw(msg); break;
        case 9:  servoValue = mavlink_msg_servo_output_raw_get_servo9_raw(msg); break;
        case 10: servoValue = mavlink_msg_servo_output_raw_get_servo10_raw(msg); break;
        case 11: servoValue = mavlink_msg_servo_output_raw_get_servo11_raw(msg); break;
        case 12: servoValue = mavlink_msg_servo_output_raw_get_servo12_raw(msg); break;
        case 13: servoValue = mavlink_msg_servo_output_raw_get_servo13_raw(msg); break;
        case 14: servoValue = mavlink_msg_servo_output_raw_get_servo14_raw(msg); break;
        case 15: servoValue = mavlink_msg_servo_output_raw_get_servo15_raw(msg); break;
        case 16: servoValue = mavlink_msg_servo_output_raw_get_servo16_raw(msg); break;
        default: return; // Invalid channel
    }
    
    // Check for invalid servo values (0 = disabled, 65535 = invalid)
    if (servoValue == 0 || servoValue == 65535) {
        _signalValid = false;
        _setFailsafe();
        return;
    }
    
    // Update servo value and timestamp
    _servoValue = servoValue;
    _lastServoUpdate = millis();
    _signalValid = true;
    
    // Update PWM output
    _updatePWMOutput();
}

//---------------------------------------------------------------------------------
void
MavESP8266PPM::_updatePWMOutput()
{
    if (!_signalValid) {
        return;
    }
    
    // Map servo value (1000-2000us) to duty cycle (0-4095)
    _dutyCycle = _mapServoValueToDuty(_servoValue);
    
    // Write to LEDC
    ledcWrite(PWM_CHANNEL, _dutyCycle);
}

//---------------------------------------------------------------------------------
void
MavESP8266PPM::_setFailsafe()
{
    // Stop motor (0% duty cycle)
    _dutyCycle = 0;
    ledcWrite(PWM_CHANNEL, 0);
}

//---------------------------------------------------------------------------------
uint16_t
MavESP8266PPM::_mapServoValueToDuty(uint16_t servoValue)
{
    // Clamp servo value to valid range
    if (servoValue < SERVO_MIN_PULSE) {
        servoValue = SERVO_MIN_PULSE;
    } else if (servoValue > SERVO_MAX_PULSE) {
        servoValue = SERVO_MAX_PULSE;
    }
    
    // Linear mapping: 1000us = 0%, 2000us = 100%
    // duty = (servoValue - 1000) * 255 / 1000
    uint32_t duty = ((uint32_t)(servoValue - SERVO_MIN_PULSE) * ((1 << PWM_RESOLUTION) - 1)) / 
                    (SERVO_MAX_PULSE - SERVO_MIN_PULSE);
    
    return (uint16_t)duty;
}

