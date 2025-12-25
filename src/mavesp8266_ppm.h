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
 * ArduPilot Servo PWM to Motor Controller PWM Converter
 * Converts standard servo PWM (1000-2000us @ 50Hz) to 16kHz PWM for motor controller
 *
 */

#ifndef MAVESP8266_PPM_H
#define MAVESP8266_PPM_H

#include <Arduino.h>

// GPIO Pin Configuration
#define PWM_INPUT_PIN   1    // GPIO1 - ArduPilot servo PWM input
#define PWM_OUTPUT_PIN  14   // GPIO14 - 16kHz PWM output to motor controller (GPIO2 conflicts with factory reset)

// PWM Output Configuration (16kHz for motor controller)
#define PWM_FREQ        16000   // 16kHz output frequency
#define PWM_RESOLUTION  12      // 12-bit resolution (0-4095)
#define PWM_CHANNEL     0       // LEDC channel 0

// Input Signal Configuration
#define PWM_MIN_PULSE   1000    // Minimum valid pulse width (us)
#define PWM_MAX_PULSE   2000    // Maximum valid pulse width (us)
#define PWM_TIMEOUT_MS  100     // Signal loss timeout (ms)

class MavESP8266PPM
{
public:
    MavESP8266PPM();
    ~MavESP8266PPM();
    
    void    begin();
    void    update();
    
    // Getters for monitoring
    uint16_t getPulseWidth()    { return _pulseWidth; }
    uint16_t getDutyCycle()     { return _dutyCycle; }
    bool     isSignalValid()    { return _signalValid; }
    
private:
    // Signal processing
    void        _processPulse();
    void        _updatePWMOutput();
    void        _setFailsafe();
    uint16_t    _mapPulseToDuty(uint16_t pulseWidth);
    
    // Interrupt handler (must be static)
    static void IRAM_ATTR _handleInterrupt();
    
    // State variables
    volatile unsigned long  _lastRiseTime;
    volatile unsigned long  _pulseWidthRaw;
    volatile bool           _newPulse;
    
    uint16_t    _pulseWidth;        // Filtered pulse width (us)
    uint16_t    _dutyCycle;         // Current duty cycle (0-4095)
    uint32_t    _lastValidPulse;    // Last valid pulse timestamp (ms)
    bool        _signalValid;       // Signal valid flag
    bool        _initialized;       // Initialization flag
};

#endif // MAVESP8266_PPM_H
