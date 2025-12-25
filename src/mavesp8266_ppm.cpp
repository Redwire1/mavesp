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
 * ArduPilot Servo PWM to Motor Controller PWM Converter
 *
 */

#include "mavesp8266_ppm.h"

// Static instance pointer for ISR access
static MavESP8266PPM* _instance = nullptr;

//---------------------------------------------------------------------------------
MavESP8266PPM::MavESP8266PPM()
    : _lastRiseTime(0)
    , _pulseWidthRaw(0)
    , _newPulse(false)
    , _pulseWidth(1500)
    , _dutyCycle(0)
    , _lastValidPulse(0)
    , _signalValid(false)
    , _initialized(false)
{
    _instance = this;
}

//---------------------------------------------------------------------------------
MavESP8266PPM::~MavESP8266PPM()
{
    if (_initialized) {
        // Interrupt-based approach disabled, nothing to detach
        // detachInterrupt(PWM_INPUT_PIN);
    }
    _instance = nullptr;
}

//---------------------------------------------------------------------------------
void
MavESP8266PPM::begin()
{
#ifdef ARDUINO_ARCH_ESP32
    // Configure input pin with PULLDOWN to prevent floating state
    pinMode(PWM_INPUT_PIN, INPUT_PULLDOWN);
    
    // Configure LEDC for 16kHz PWM output
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PWM_OUTPUT_PIN, PWM_CHANNEL);
    
    // Start with motor off (0% duty cycle)
    ledcWrite(PWM_CHANNEL, 0);
    _dutyCycle = 0;
    
    // NOTE: Interrupt-based reading disabled - causes WiFi crashes
    // Use polling-based approach instead (safer with WiFi)
    // attachInterrupt(PWM_INPUT_PIN, _handleInterrupt, CHANGE);
    
    _initialized = true;
    _lastValidPulse = millis();
    
    Serial.println("PWM Converter initialized: GPIO1 input -> GPIO2 output @ 16kHz (polling mode)");
#else
    Serial.println("PWM Converter only supported on ESP32");
#endif
}

//---------------------------------------------------------------------------------
void
MavESP8266PPM::update()
{
    if (!_initialized) {
        return;
    }
    
    // Polling-based PWM reading (WiFi-safe, no interrupts)
    static unsigned long lastRiseTime = 0;
    static bool lastState = false;
    
    bool currentState = digitalRead(PWM_INPUT_PIN);
    unsigned long now = micros();
    
    // Detect rising edge
    if (currentState && !lastState) {
        lastRiseTime = now;
    }
    // Detect falling edge
    else if (!currentState && lastState && lastRiseTime > 0) {
        unsigned long pulseWidth = now - lastRiseTime;
        
        // Validate pulse width is in expected range
        if (pulseWidth >= PWM_MIN_PULSE && pulseWidth <= PWM_MAX_PULSE) {
            _pulseWidth = (uint16_t)pulseWidth;
            _lastValidPulse = millis();
            _signalValid = true;
            _processPulse();
        }
        
        lastRiseTime = 0;
    }
    
    lastState = currentState;
    
    // Check for signal timeout
    if (_signalValid && (millis() - _lastValidPulse > PWM_TIMEOUT_MS)) {
        _setFailsafe();
    }
}

//---------------------------------------------------------------------------------
void
MavESP8266PPM::_processPulse()
{
    // Map pulse width to duty cycle
    uint16_t newDuty = _mapPulseToDuty(_pulseWidth);
    
    // Update output if changed
    if (newDuty != _dutyCycle) {
        _dutyCycle = newDuty;
        _updatePWMOutput();
    }
}

//---------------------------------------------------------------------------------
void
MavESP8266PPM::_updatePWMOutput()
{
#ifdef ARDUINO_ARCH_ESP32
    ledcWrite(PWM_CHANNEL, _dutyCycle);
#endif
}

//---------------------------------------------------------------------------------
void
MavESP8266PPM::_setFailsafe()
{
    // Signal lost - set motor to 0% (off)
    _signalValid = false;
    _dutyCycle = 0;
    _updatePWMOutput();
    
    Serial.println("PWM Converter: Signal lost - failsafe activated (motor off)");
}

//---------------------------------------------------------------------------------
uint16_t
MavESP8266PPM::_mapPulseToDuty(uint16_t pulseWidth)
{
    // Map 1000-2000us input to 0-4095 duty cycle (12-bit)
    // 1000us = 0% duty = 0
    // 2000us = 100% duty = 4095
    
    // Constrain input
    if (pulseWidth < PWM_MIN_PULSE) pulseWidth = PWM_MIN_PULSE;
    if (pulseWidth > PWM_MAX_PULSE) pulseWidth = PWM_MAX_PULSE;
    
    // Linear mapping
    uint32_t duty = ((uint32_t)(pulseWidth - PWM_MIN_PULSE) * 4095UL) / (PWM_MAX_PULSE - PWM_MIN_PULSE);
    
    return (uint16_t)duty;
}

// Old interrupt-based code - disabled to prevent WiFi crashes
// Kept for reference - interrupts conflict with WiFi on ESP32
/*
//---------------------------------------------------------------------------------
// Interrupt Service Routine - must be in IRAM and minimal code
void IRAM_ATTR
MavESP8266PPM::_handleInterrupt()
{
    if (_instance == nullptr) {
        return;
    }
    
    unsigned long now = micros();
    
    // Debounce: Ignore triggers within 20us of last interrupt (noise/bounce)
    static unsigned long lastInterruptTime = 0;
    if (now - lastInterruptTime < 20) {
        return;
    }
    lastInterruptTime = now;
    
    // Use direct GPIO register read instead of digitalRead()
    bool pinState = (GPIO.in >> PWM_INPUT_PIN) & 0x1;
    
    if (pinState) {
        // Rising edge - start of pulse
        _instance->_lastRiseTime = now;
    } else {
        // Falling edge - end of pulse
        if (_instance->_lastRiseTime > 0) {
            unsigned long pulseWidth = now - _instance->_lastRiseTime;
            
            // Basic validation in ISR (avoid processing garbage)
            if (pulseWidth >= 500 && pulseWidth <= 2500) {
                _instance->_pulseWidthRaw = pulseWidth;
                _instance->_newPulse = true;
            }
            _instance->_lastRiseTime = 0; // Reset for next pulse
        }
    }
}
*/
