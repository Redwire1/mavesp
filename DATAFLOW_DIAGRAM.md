```mermaid
flowchart TB
    subgraph External["External Systems"]
        Vehicle["Vehicle/Autopilot<br/>(Serial UART)"]
        GCS["Ground Control Station<br/>(UDP WiFi)"]
    end
    
    subgraph ESP32["ESP32 Device"]
        subgraph VehicleLink["Vehicle Communication"]
            VehicleRead["MavESP8266Vehicle::readMessage()"]
            VehicleParser["mavlink_frame_char_buffer()"]
            VehicleSend["MavESP8266Vehicle::sendMessage()"]
        end
        
        subgraph GCSLink["GCS Communication"]
            GCSRead["MavESP8266GCS::readMessage()"]
            GCSParser["mavlink_frame_char_buffer()"]
            GCSSend["MavESP8266GCS::sendMessage()"]
        end
        
        subgraph MessageHandler["Message Processing"]
            Component["MavESP8266Component::handleMessage()"]
            ParamSet["Handle PARAM_SET"]
            CmdLong["Handle COMMAND_LONG"]
        end
        
        subgraph PWMExtraction["PWM Servo Channel Extraction"]
            ServoMsg["SERVO_OUTPUT_RAW<br/>(MAVLink Msg ID 36)"]
            PPMHandler["MavESP8266PPM::handleServoOutput()"]
            ChannelExtract["Extract Configured Channel<br/>(5-16, default 5)"]
            ServoValue["Servo Value<br/>(1000-2000 µs)"]
            DutyMap["_mapServoValueToDuty()"]
            DutyCycle["Duty Cycle<br/>(0-4095, 12-bit)"]
            LEDC["ledcWrite()<br/>16kHz PWM Output"]
        end
        
        subgraph Output["Hardware Output"]
            GPIO14["GPIO 14<br/>PWM to Motor Controller"]
        end
    end
    
    Vehicle -->|"Serial Data"| VehicleRead
    VehicleRead -->|"Parse Bytes"| VehicleParser
    VehicleParser -->|"Valid MAVLink Message"| Component
    Component -->|"Check if handled"| GCSSend
    GCSSend -->|"Forward to GCS"| GCS
    
    GCS -->|"UDP Packets"| GCSRead
    GCSRead -->|"Parse Bytes"| GCSParser
    GCSParser -->|"Valid MAVLink Message"| Component
    Component -->|"Check if handled"| VehicleSend
    VehicleSend -->|"Forward to Vehicle"| Vehicle
    
    Component -->|"PARAM_SET<br/>target=UDP_BRIDGE"| ParamSet
    Component -->|"COMMAND_LONG<br/>target=UDP_BRIDGE"| CmdLong
    
    VehicleParser -->|"Message ID 36"| ServoMsg
    ServoMsg --> PPMHandler
    PPMHandler --> ChannelExtract
    ChannelExtract -->|"servo5_raw to servo16_raw"| ServoValue
    ServoValue -->|"Validate 1000-2000µs"| DutyMap
    DutyMap -->|"Linear mapping"| DutyCycle
    DutyCycle --> LEDC
    LEDC --> GPIO14
    
    click VehicleRead call linkCallback("c:/Users/simon/OneDrive/Personal Projects/Ardupilot ESP32 Mavelink2 Webserver/mavesp/src/mavesp8266_vehicle.cpp#L80")
    click VehicleSend call linkCallback("c:/Users/simon/OneDrive/Personal Projects/Ardupilot ESP32 Mavelink2 Webserver/mavesp/src/mavesp8266_vehicle.cpp#L115")
    click GCSRead call linkCallback("c:/Users/simon/OneDrive/Personal Projects/Ardupilot ESP32 Mavelink2 Webserver/mavesp/src/mavesp8266_gcs.cpp#L67")
    click GCSSend call linkCallback("c:/Users/simon/OneDrive/Personal Projects/Ardupilot ESP32 Mavelink2 Webserver/mavesp/src/mavesp8266_gcs.cpp")
    click Component call linkCallback("c:/Users/simon/OneDrive/Personal Projects/Ardupilot ESP32 Mavelink2 Webserver/mavesp/src/mavesp8266_component.cpp#L67")
    click PPMHandler call linkCallback("c:/Users/simon/OneDrive/Personal Projects/Ardupilot ESP32 Mavelink2 Webserver/mavesp/src/mavesp8266_ppm.cpp#L106")
    click DutyMap call linkCallback("c:/Users/simon/OneDrive/Personal Projects/Ardupilot ESP32 Mavelink2 Webserver/mavesp/src/mavesp8266_ppm.cpp#L169")
    
    classDef external fill:#e1f5ff,stroke:#0066cc,stroke-width:2px
    classDef vehicle fill:#fff4e6,stroke:#ff9800,stroke-width:2px
    classDef gcs fill:#e8f5e9,stroke:#4caf50,stroke-width:2px
    classDef handler fill:#f3e5f5,stroke:#9c27b0,stroke-width:2px
    classDef pwm fill:#fff3e0,stroke:#ff6f00,stroke-width:3px
    classDef output fill:#ffebee,stroke:#d32f2f,stroke-width:3px
    
    class Vehicle,GCS external
    class VehicleRead,VehicleParser,VehicleSend vehicle
    class GCSRead,GCSParser,GCSSend gcs
    class Component,ParamSet,CmdLong handler
    class ServoMsg,PPMHandler,ChannelExtract,ServoValue,DutyMap,DutyCycle,LEDC pwm
    class GPIO14 output
```