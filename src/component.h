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
 * @file component.h
 * ESP8266 Wifi AP, MavLink UART/UDP Bridge
 *
 * @author Gus Grubba <mavlink@grubba.com>
 */

#ifndef COMPONENT_H
#define COMPONENT_H

#include "bridge.h"

class Component {
public:
    Component();

    //- Returns true if the component consumed the message
    bool handleMessage        (Bridge* sender, mavlink_message_t* message);
    bool inRawMode            ();
    void resetRawMode         () { _in_raw_mode_time = millis(); }

private:
    void    _sendStatusMessage      (Bridge* sender, uint8_t type, const char* text);
    void    _handleParamSet         (Bridge* sender, mavlink_param_set_t* param);
    void    _handleParamRequestList (Bridge* sender);
    void    _handleParamRequestRead (Bridge* sender, mavlink_param_request_read_t* param);
    void    _sendParameter          (Bridge* sender, uint16_t index);
    void    _sendParameter          (Bridge* sender, const char* id, uint32_t value, uint16_t index);

    void    _handleCmdLong          (Bridge* sender, mavlink_command_long_t* cmd, uint8_t compID);

    void    _wifiReboot             (Bridge* sender);

    bool            _in_raw_mode;
    unsigned long   _in_raw_mode_time;
};

#endif
