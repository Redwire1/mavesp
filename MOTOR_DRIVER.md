# ZS-X11H Motor Controller Integration

## Overview

This project implements a MAVLink to PWM converter that extracts servo output commands from ArduPilot and translates them into PWM signals suitable for driving a ZS-X11H hoverboard motor controller. The system acts as a bridge between an autopilot system and a brushless motor via a commercial hoverboard controller.

## Hardware: ZS-X11H Motor Controller

### Description
The **ZS-X11H** is a compact brushless motor controller commonly found in hoverboards. It's designed to control high-power brushless DC (BLDC) motors using simple PWM input signals and provides built-in motor commutation, current limiting, and protection features.

**Key Features:**
- **Voltage Range:** 24V-60V DC (typical hoverboard configurations)
- **Power Rating:** Up to 350W continuous (500W peak)
- **Control Interface:** 490Hz PWM input (0-100% duty cycle)
- **Direction Control:** Dedicated DIR pin (HIGH/LOW)
- **Motor Type:** 3-phase BLDC motors with Hall sensor feedback
- **Built-in Protection:** Over-current, over-voltage, under-voltage, over-temperature

### Pin Interface

| Pin | Function | Description |
|-----|----------|-------------|
| **PWM** | Speed Input | 490Hz PWM signal (0-100% duty = 0-100% speed) |
| **DIR** | Direction | Digital HIGH/LOW for rotation direction |
| **+5V** | Logic Power | 5V power supply for controller logic |
| **GND** | Ground | Common ground reference |
| **BATT+** | Motor Power | Battery positive (24-60V) |
| **BATT-** | Motor Ground | Battery negative |
| **M1, M2, M3** | Motor Phases | 3-phase motor connections |
| **H1, H2, H3** | Hall Sensors | Motor position feedback inputs |

### PWM Requirements
- **Frequency:** 490Hz (±10Hz tolerance)
- **Duty Cycle Range:** 0-100%
  - 0% = Motor stopped
  - 1-10% = Startup threshold (motor may not spin)
  - 10-100% = Linear speed control
- **Logic Level:** 3.3V or 5V compatible
- **Resolution:** Minimum 8-bit (0-255) recommended

## System Architecture

```
ArduPilot/PX4 → MAVLink → ESP32 → 490Hz PWM → ZS-X11H → BLDC Motor
  (Autopilot)   (Serial)  (Bridge)   (GPIO14)  (Controller) (Hoverboard)
```

### Data Flow

1. **ArduPilot** generates servo output commands (servo channels 5-16)
2. **MAVLink** transmits `SERVO_OUTPUT_RAW` messages over serial (~50Hz)
3. **ESP32** extracts servo value from configured channel (1000-2000µs)
4. **PWM Converter** maps servo value to PWM duty cycle (0-100%)
5. **ZS-X11H** drives the motor based on PWM input
6. **Failsafe** triggers if no MAVLink signal received for >1 second

## Implementation Details

### File Structure

- **[mavesp8266_ppm.h](src/mavesp8266_ppm.h)** - Class definition and configuration
- **[mavesp8266_ppm.cpp](src/mavesp8266_ppm.cpp)** - Implementation and signal processing

### Key Classes

#### `MavESP8266PPM`
Main class responsible for MAVLink servo signal extraction and PWM generation.

**Public Methods:**
```cpp
void begin()                                    // Initialize PWM hardware
void update()                                   // Check for signal timeout
void handleServoOutput(const mavlink_message_t* msg)  // Process MAVLink messages
```

**Getters:**
```cpp
uint16_t getServoValue()    // Current servo input (1000-2000µs)
uint16_t getDutyCycle()     // Current PWM duty cycle (0-255)
uint8_t  getServoChannel()  // Configured servo channel (5-16)
bool     isSignalValid()    // Signal health status
```

### Configuration Constants

```cpp
// Hardware
#define PWM_OUTPUT_PIN  14          // GPIO14 output to ZS-X11H

// PWM Generation (490Hz for ZS-X11H)
#define PWM_FREQ        490         // Frequency in Hz
#define PWM_RESOLUTION  8           // 8-bit = 0-255 range
#define PWM_CHANNEL     0           // ESP32 LEDC channel

// Servo Input Validation
#define SERVO_MIN_PULSE     1000    // Minimum servo value (µs)
#define SERVO_MAX_PULSE     2000    // Maximum servo value (µs)
#define SERVO_TIMEOUT_MS    1000    // Signal loss timeout (ms)
```

### Signal Processing Pipeline

#### 1. MAVLink Message Reception
```cpp
void handleServoOutput(const mavlink_message_t* msg)
```
- Receives `SERVO_OUTPUT_RAW` messages from ArduPilot
- Extracts servo value from configured channel (5-16)
- Validates servo value (rejects 0, 65535, or out-of-range values)

#### 2. Servo Value Mapping
```cpp
uint16_t _mapServoValueToDuty(uint16_t servoValue)
```
Converts servo input to PWM duty cycle:

**Input:** 1000-2000µs servo pulse width  
**Output:** 0-255 PWM duty cycle value

**Mapping Formula:**
```
duty = (servoValue - 1000) × 255 ÷ 1000
```

**Examples:**
- 1000µs → 0 (motor stopped)
- 1500µs → 127 (50% speed)
- 2000µs → 255 (100% speed)

#### 3. PWM Output Generation
```cpp
void _updatePWMOutput()
```
- Uses ESP32 LEDC (LED Control) peripheral
- Generates 490Hz PWM signal on GPIO14
- Updates duty cycle based on mapped servo value

#### 4. Failsafe Handling
```cpp
void _setFailsafe()
```
Triggers when:
- No `SERVO_OUTPUT_RAW` received for >1000ms
- Invalid servo value detected (0 or 65535)
- Signal quality degrades

**Failsafe Action:**
- Sets PWM duty cycle to 0%
- Motor controller stops the motor
- Clears signal valid flag

### Usage Example

```cpp
#include "mavesp8266_ppm.h"

MavESP8266PPM pwmConverter;

void setup() {
    pwmConverter.begin();  // Initialize 490Hz PWM on GPIO14
}

void loop() {
    pwmConverter.update();  // Check for timeouts
    
    // When MAVLink SERVO_OUTPUT_RAW message received:
    pwmConverter.handleServoOutput(&msg);
    
    // Monitor status
    if (pwmConverter.isSignalValid()) {
        uint16_t servo = pwmConverter.getServoValue();  // 1000-2000
        uint16_t duty = pwmConverter.getDutyCycle();    // 0-255
        // Motor is running
    } else {
        // Failsafe active - motor stopped
    }
}
```

## Parameter Configuration

The servo channel can be configured via MAVLink parameters:

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| `PWM_SERVO_CHANNEL` | 5-16 | 5 | Servo output channel to monitor |

**ArduPilot Configuration:**
```
SERVO5_FUNCTION = 70    # Throttle output
SERVO5_MIN = 1000       # Minimum PWM
SERVO5_MAX = 2000       # Maximum PWM
SERVO5_TRIM = 1000      # Neutral position
```

## Hardware Connections

### ESP32 to ZS-X11H Wiring

```
ESP32                    ZS-X11H Controller
┌──────────────┐        ┌──────────────┐
│              │        │              │
│ GPIO14 ──────┼────────┼─→ PWM Input  │
│              │        │              │
│ GND ─────────┼────────┼─→ GND        │
│              │        │              │
│ 5V (opt) ────┼────────┼─→ +5V Logic  │
│              │        │              │
└──────────────┘        └──────────────┘
                               │
                        ┌──────┴───────┐
                        │ BLDC Motor   │
                        │ (Hoverboard) │
                        └──────────────┘
```

**Important Notes:**
1. **Common Ground:** ESP32 and ZS-X11H must share a common ground
2. **Logic Level:** GPIO14 outputs 3.3V PWM (ZS-X11H is 3.3V/5V compatible)
3. **Power Isolation:** ESP32 powered separately from motor battery
4. **Signal Quality:** Keep PWM wire short (<30cm) to minimize noise

### Complete System Diagram

```
┌─────────────┐      ┌───────────────┐      ┌──────────────┐
│  ArduPilot  │ UART │    ESP32      │ PWM  │  ZS-X11H     │
│  Autopilot  ├──────┤  MAVLink to   ├──────┤  Motor       │
│             │ TX/RX│  PWM Converter│GPIO14│  Controller  │
└─────────────┘      └───────────────┘      └──────┬───────┘
      ↑                     ↑                       │
      │                     │                    3-Phase
   Control              USB/WiFi               ┌───┴────┐
   Commands             Monitoring             │  BLDC  │
                                               │  Motor │
                                               └────────┘
```

## Safety Features

### 1. Signal Validation
- Rejects invalid servo values (0, 65535)
- Clamps values to 1000-2000µs range
- Validates message timing and integrity

### 2. Timeout Protection
- Monitors MAVLink message rate (~50Hz expected)
- Triggers failsafe if no signal for 1 second
- Automatically recovers when signal returns

### 3. Failsafe Mode
- Sets motor to 0% duty cycle (full stop)
- No partial throttle in failsafe (safer than idle)
- Visual indication via signal valid flag

### 4. Controlled Startup
- Motor starts at 0% on boot
- No unexpected motion during initialization
- Requires valid MAVLink signal to operate

## Testing & Calibration

### 1. Bench Test (No Load)
```cpp
// Test sequence without motor connected
1. Power ESP32 and verify initialization
2. Send MAVLink SERVO_OUTPUT_RAW commands
3. Verify GPIO14 outputs 490Hz PWM
4. Confirm duty cycle mapping (oscilloscope)
5. Test failsafe triggers after 1 second
```

### 2. Motor Test (With Load)
```
1. Connect ZS-X11H to motor
2. Start with low throttle (1100-1200µs)
3. Gradually increase to verify response
4. Test full range (1000-2000µs)
5. Verify motor stops on signal loss
```

### 3. Calibration Steps
1. **Find Minimum Threshold:** Determine lowest PWM where motor spins
2. **Map Dead Zone:** Adjust `SERVO_MIN_PULSE` if needed
3. **Verify Max Speed:** Ensure 2000µs = full motor capability
4. **Tune Response:** Adjust ArduPilot servo output scaling

### 4. Performance Validation
- **Response Time:** <20ms from MAVLink to motor change
- **Update Rate:** 50Hz MAVLink = smooth motor control
- **Signal Quality:** <1% duty cycle jitter
- **Failsafe Latency:** <1 second detection

## Reference Implementation

This implementation is based on best practices from the hoverboard motor community:

**Reference Project:** [PID-for-Hoverboard-motor-with-ZS-X11H-controller](https://github.com/oracid/PID-for-Hoverboard-motor-with-ZS-X11H-controller)
- Demonstrates 490Hz PWM output to ZS-X11H
- Shows Hall sensor feedback integration
- Provides PID control examples
- Documents hardware connections

**Key Takeaways:**
- 490Hz PWM is critical (not 50Hz servo signal)
- 8-bit resolution (0-255) is sufficient
- Motor startup requires >10% duty cycle
- Hall sensors can provide speed feedback

## Troubleshooting

### Motor Not Spinning
- **Check PWM frequency:** Must be 490Hz (verify with oscilloscope)
- **Verify connections:** GPIO14 → PWM, GND → GND
- **Increase throttle:** Motor may need >10% to overcome startup
- **Check direction pin:** Ensure DIR pin is properly set (not floating)

### Erratic Motor Behavior
- **Signal integrity:** Shorten PWM wire, add ground plane
- **Power supply:** Ensure stable voltage to ZS-X11H logic
- **MAVLink rate:** Confirm 50Hz `SERVO_OUTPUT_RAW` messages
- **Duty cycle mapping:** Verify 1000-2000µs → 0-255 conversion

### Failsafe Not Triggering
- **Timeout value:** Reduce `SERVO_TIMEOUT_MS` if needed
- **Message rate:** Ensure MAVLink stream is active
- **Update loop:** Confirm `update()` called regularly

### PWM Not Generating
- **ESP32 LEDC:** Verify LEDC channel not in use elsewhere
- **Pin conflict:** Check GPIO14 not reassigned
- **Initialization:** Ensure `begin()` called before use

## Future Enhancements

### Potential Improvements
1. **Bi-directional Control:** Add DIR pin support for reversing
2. **Hall Sensor Feedback:** Read motor speed via Hall interrupts
3. **Closed-Loop Control:** Implement PID for precise speed control
4. **Telemetry:** Send motor RPM back to ArduPilot
5. **Current Monitoring:** Measure motor current for load estimation
6. **Soft Start:** Gradual ramp-up to prevent mechanical shock

### Advanced Features
- **Motor Temperature:** Monitor ZS-X11H thermal state
- **Fault Detection:** Decode controller error states
- **Multiple Motors:** Support simultaneous control
- **Dynamic Frequency:** Adjust PWM frequency for different controllers

## Additional Resources

- **ArduPilot Servo Output:** [Servo Output Raw Message](https://mavlink.io/en/messages/common.html#SERVO_OUTPUT_RAW)
- **ESP32 LEDC Tutorial:** [ESP32 LED PWM Controller](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/ledc.html)
- **Hoverboard Hacking:** [Community Wiki](https://github.com/EmanuelFeru/hoverboard-firmware-hack-FOC)
- **ZS-X11H Reference:** [PID Control Implementation](https://github.com/oracid/PID-for-Hoverboard-motor-with-ZS-X11H-controller)

## License

Copyright (c) 2025. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the conditions in the source files are met.

---

**Last Updated:** December 26, 2025  
**Hardware Version:** ZS-X11H (Generic Hoverboard Controller)  
**Software Version:** ESP32 Arduino Framework  
**Target Platform:** ESP32-S3-CAM
