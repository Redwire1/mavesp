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
 * @file bridge.h
 * ESP8266/ESP32 Wifi AP, MavLink UART/UDP Bridge
 *
 * @author Gus Grubba <mavlink@grubba.com>
 */

#ifndef BRIDGE_H
#define BRIDGE_H

#ifdef ARDUINO_ARCH_ESP32
    #include <WiFi.h>
    #include <WiFiClient.h>
    #include <WiFiUdp.h>
#else
    #include <ESP8266WiFi.h>
    #include <WiFiClient.h>
    #include <WiFiUdp.h>
#endif

#undef F
#include <ardupilotmega/mavlink.h>

#ifndef ARDUINO_ARCH_ESP32
extern "C" {
    // Espressif SDK (ESP8266 only)
    #include "user_interface.h"
}
#endif

class Parameters;
class Component;
class Vehicle;
class Gcs;
class Ppm;

#define DEFAULT_UART_SPEED          921600
#define DEFAULT_WIFI_CHANNEL        11
#define DEFAULT_UDP_HPORT           14550
#define DEFAULT_UDP_CPORT           14555

#define HEARTBEAT_TIMEOUT           10 * 1000

//-- TODO: This needs to come from the build system
#define VERSION_MAJOR    1
#define VERSION_MINOR    2
#define VERSION_BUILD    3
#define VERSION          ((VERSION_MAJOR << 24) & 0xFF00000) | ((VERSION_MINOR << 16) & 0x00FF0000) | (VERSION_BUILD & 0xFFFF)

//-- Debug sent out to Serial1 (GPIO02), which is TX only (no RX).
//#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
#define DEBUG_LOG(format, ...) do { getWorld()->getLogger()->log(format, ## __VA_ARGS__); } while(0)
#else
#define DEBUG_LOG(format, ...) do { } while(0)
#endif

//---------------------------------------------------------------------------------
//-- Link Status
struct link_status_t {
    uint32_t    packets_received;
    uint32_t    packets_lost;
    uint32_t    packets_sent;
    uint32_t    parse_errors;
    uint32_t    radio_status_sent;
    uint8_t     queue_status;
    // ⚠️ TEMPORARY BRIDGE FIELDS (Increment 0-2 only, removed in Increment 3 T040)
    bool        is_armed;           // Set from HEARTBEAT MAV_MODE_FLAG_SAFETY_ARMED
    uint32_t    last_heartbeat_ms;  // millis() of last HEARTBEAT from vehicle
};

//---------------------------------------------------------------------------------
//-- Base Comm Link
class Bridge {
public:
    Bridge();
    virtual ~Bridge(){;}
    virtual void    begin           (Bridge* forwardTo);
    virtual void    readMessage     () = 0;
    virtual void    readMessageRaw  () = 0;
    virtual int     sendMessage     (mavlink_message_t* message) = 0;
    virtual int     sendMessageRaw   (uint8_t *buffer, int len) = 0;
    virtual bool    heardFrom       () { return _heard_from;    }
    virtual uint8_t systemID        () { return _system_id;     }
    virtual uint8_t componentID     () { return _component_id;  }
    virtual link_status_t* getStatus() { return &_status;       }
    mavlink_channel_t       _send_chan;
    mavlink_channel_t       _recv_chan;
protected:
    virtual void    _checkLinkErrors(mavlink_message_t* msg);
protected:
    bool                    _heard_from;
    uint8_t                 _system_id;
    uint8_t                 _component_id;
    uint8_t                 _seq_expected;
    uint32_t                _last_heartbeat;
    link_status_t           _status;
    unsigned long           _last_status_time;
    Bridge*                 _forwardTo;
    mavlink_status_t        _mav_status;
    mavlink_message_t       _rxmsg;
    mavlink_status_t        _rxstatus;

    void handle_non_mavlink(uint8_t b, bool msgReceived);
    uint8_t _non_mavlink_buffer[270];
    uint16_t _non_mavlink_len;
    mavlink_parse_state_t _last_parse_state;
};

//---------------------------------------------------------------------------------
//-- Logger
class Log {
public:
    Log   ();
    void            begin           (size_t bufferSize); // Allocate a buffer for the log
    size_t          log             (const char *format, ...); // Add to the log
    String          getLog          (uint32_t* pStart, uint32_t* pLen); // Get the log starting at a position
    uint32_t        getLogSize      (); // Number of bytes available at the current log position
    uint32_t        getPosition     ();
private:
    char*           _buffer; // Raw memory
    size_t          _buffer_size; // Size of the above memory
    uint32_t        _log_offset; // Position in the buffer
    uint32_t        _log_position; // Absolute position in the log since boot
};

//---------------------------------------------------------------------------------
//-- Accessors
class World {
public:
    virtual ~World(){;}
    virtual Parameters* getParameters  () = 0;
    virtual Component*  getComponent   () = 0;
    virtual Vehicle*    getVehicle     () = 0;
    virtual Gcs*        getGCS         () = 0;
    virtual Log*        getLogger      () = 0;
    virtual Ppm*        getPWM         () = 0;
};

//---------------------------------------------------------------------------------
//-- HTTP Update Status
class OtaUpdate {
public:
    virtual ~OtaUpdate(){;}
    virtual void updateStarted  () = 0;
    virtual void updateCompleted() = 0;
    virtual void updateError    () = 0;
};

extern World* getWorld();

#endif
