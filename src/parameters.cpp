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
 * @file parameters.cpp
 * ESP8266 Wifi AP, MavLink UART/UDP Bridge
 *
 * @author Gus Grubba <mavlink@grubba.com>
 */

#include <Arduino.h>
#include <EEPROM.h>
#include "bridge.h"
#include "parameters.h"
#include "crc.h"

const char* kDEFAULT_SSID       = "ArduPilot";
const char* kDEFAULT_PASSWORD   = "ardupilot";

//-- Reserved space for EEPROM persistence. A change in this will cause all values to reset to defaults.
#define EEPROM_SPACE            32 * sizeof(uint32_t)
#define EEPROM_CRC_ADD          EEPROM_SPACE - (sizeof(uint32_t) << 1)

uint32_t    _sw_version;
int8_t      _debug_enabled;
int8_t      _wifi_mode;
uint32_t    _wifi_channel;
uint16_t    _wifi_udp_hport;
uint16_t    _wifi_udp_cport;
uint32_t    _wifi_ip_address;
char        _wifi_ssid[16];
char        _wifi_password[16];
char        _wifi_ssidsta[16];
char        _wifi_passwordsta[16];
uint32_t    _wifi_ipsta;
uint32_t    _wifi_gatewaysta;
uint32_t    _wifi_subnetsta;
uint32_t    _uart_baud_rate;
uint32_t    _flash_left;
int8_t      _raw_enable;
uint8_t      _pwm_servo_channel;

//-- Parameters
//   No string support in parameters so we stash a char[16] into 4 uint32_t
 struct parameters_t mavParameters[] = {
     {"SW_VER",             &_sw_version,           Parameters::ID_FWVER,     sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  true },
     {"DEBUG_ENABLED",      &_debug_enabled,        Parameters::ID_DEBUG,     sizeof(int8_t),     MAV_PARAM_TYPE_INT8,    false},
     {"WIFI_MODE",          &_wifi_mode,            Parameters::ID_MODE,      sizeof(int8_t),     MAV_PARAM_TYPE_INT8,    false},
     {"WIFI_CHANNEL",       &_wifi_channel,         Parameters::ID_CHANNEL,   sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_UDP_HPORT",     &_wifi_udp_hport,       Parameters::ID_HPORT,     sizeof(uint16_t),   MAV_PARAM_TYPE_UINT16,  false},
     {"WIFI_UDP_CPORT",     &_wifi_udp_cport,       Parameters::ID_CPORT,     sizeof(uint16_t),   MAV_PARAM_TYPE_UINT16,  false},
     {"WIFI_IPADDRESS",     &_wifi_ip_address,      Parameters::ID_IPADDRESS, sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  true },
     {"WIFI_SSID1",         &_wifi_ssid[0],         Parameters::ID_SSID1,     sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_SSID2",         &_wifi_ssid[4],         Parameters::ID_SSID2,     sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_SSID3",         &_wifi_ssid[8],         Parameters::ID_SSID3,     sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_SSID4",         &_wifi_ssid[12],        Parameters::ID_SSID4,     sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_PASSWORD1",     &_wifi_password[0],     Parameters::ID_PASS1,     sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_PASSWORD2",     &_wifi_password[4],     Parameters::ID_PASS2,     sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_PASSWORD3",     &_wifi_password[8],     Parameters::ID_PASS3,     sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_PASSWORD4",     &_wifi_password[12],    Parameters::ID_PASS4,     sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_SSIDSTA1",      &_wifi_ssidsta[0],      Parameters::ID_SSIDSTA1,  sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_SSIDSTA2",      &_wifi_ssidsta[4],      Parameters::ID_SSIDSTA2,  sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_SSIDSTA3",      &_wifi_ssidsta[8],      Parameters::ID_SSIDSTA3,  sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_SSIDSTA4",      &_wifi_ssidsta[12],     Parameters::ID_SSIDSTA4,  sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_PWDSTA1",       &_wifi_passwordsta[0],  Parameters::ID_PASSSTA1,  sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_PWDSTA2",       &_wifi_passwordsta[4],  Parameters::ID_PASSSTA2,  sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_PWDSTA3",       &_wifi_passwordsta[8],  Parameters::ID_PASSSTA3,  sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_PWDSTA4",       &_wifi_passwordsta[12], Parameters::ID_PASSSTA4,  sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_IPSTA",         &_wifi_ipsta,           Parameters::ID_IPSTA,     sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_GATEWAYSTA",    &_wifi_gatewaysta,      Parameters::ID_GATEWAYSTA,sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"WIFI_SUBNET_STA",    &_wifi_subnetsta,       Parameters::ID_SUBNETSTA, sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"UART_BAUDRATE",      &_uart_baud_rate,       Parameters::ID_UART,      sizeof(uint32_t),   MAV_PARAM_TYPE_UINT32,  false},
     {"RAW_ENABLE",         &_raw_enable,           Parameters::ID_RAW_ENABLE,sizeof(int8_t),     MAV_PARAM_TYPE_INT8,    false},
     {"PWM_SERVO_CH",       &_pwm_servo_channel,    Parameters::ID_PWM_SERVO_CHAN, sizeof(uint8_t), MAV_PARAM_TYPE_UINT8, false},
};

//---------------------------------------------------------------------------------
Parameters::Parameters()
{
}

//---------------------------------------------------------------------------------
//-- Fail safe
uint32_t bogusVar = 0;
struct parameters_t bogus = {"ERROR", &bogusVar, Parameters::ID_COUNT, sizeof(uint32_t), MAV_PARAM_TYPE_UINT32, true};

//---------------------------------------------------------------------------------
//-- Initialize
void
Parameters::begin()
{
    EEPROM.begin(EEPROM_SPACE);
    _initEeprom();
}

//---------------------------------------------------------------------------------
//-- Initialize
void
Parameters::setLocalIPAddress(uint32_t ipAddress)
{
    _wifi_ip_address = ipAddress;
}

//---------------------------------------------------------------------------------
//-- Array accessor
parameters_t*
Parameters::getAt(int index)
{
    if(index < ID_COUNT)
        return &mavParameters[index];
    else
        return &bogus;
}

//---------------------------------------------------------------------------------
//-- Parameters
uint32_t    Parameters::getSwVersion      () { return _sw_version;        }
int8_t      Parameters::getDebugEnabled   () { return _debug_enabled;     }
int8_t      Parameters::getWifiMode       () { return _wifi_mode;         }
uint32_t    Parameters::getWifiChannel    () { return _wifi_channel;      }
uint16_t    Parameters::getWifiUdpHport   () { return _wifi_udp_hport;    }
uint16_t    Parameters::getWifiUdpCport   () { return _wifi_udp_cport;    }
char*       Parameters::getWifiSsid       () { return _wifi_ssid;         }
char*       Parameters::getWifiPassword   () { return _wifi_password;     }
char*       Parameters::getWifiStaSsid    () { return _wifi_ssidsta;      }
char*       Parameters::getWifiStaPassword() { return _wifi_passwordsta;  }
uint32_t    Parameters::getWifiStaIP      () { return _wifi_ipsta;        }
uint32_t    Parameters::getWifiStaGateway () { return _wifi_gatewaysta;   }
uint32_t    Parameters::getWifiStaSubnet  () { return _wifi_subnetsta;    }
uint32_t    Parameters::getUartBaudRate   () { return _uart_baud_rate;    }
int8_t      Parameters::getRawEnable      () { return _raw_enable;        }
uint8_t     Parameters::getPWMServoChannel()  { return _pwm_servo_channel;   }

//---------------------------------------------------------------------------------
//-- Reset all to defaults
void
Parameters::resetToDefaults()
{
    _sw_version        = VERSION;
    _debug_enabled     = 0;
    _wifi_mode         = DEFAULT_WIFI_MODE;
    _wifi_channel      = DEFAULT_WIFI_CHANNEL;
    _wifi_udp_hport    = DEFAULT_UDP_HPORT;
    _wifi_udp_cport    = DEFAULT_UDP_CPORT;
    _uart_baud_rate    = DEFAULT_UART_SPEED;
    _pwm_servo_channel = 5;
    _wifi_ipsta        = 0;
    _wifi_gatewaysta   = 0;
    _wifi_subnetsta    = 0;
    strncpy(_wifi_ssid,         kDEFAULT_SSID,      sizeof(_wifi_ssid));
    strncpy(_wifi_password,     kDEFAULT_PASSWORD,  sizeof(_wifi_password));
    strncpy(_wifi_ssidsta,      kDEFAULT_SSID,      sizeof(_wifi_ssidsta));
    strncpy(_wifi_passwordsta,  kDEFAULT_PASSWORD,  sizeof(_wifi_passwordsta));
#ifdef ARDUINO_ARCH_ESP32
    _flash_left = ESP.getSketchSize();
#else
    _flash_left = ESP.getFreeSketchSpace();
#endif
}

//---------------------------------------------------------------------------------
//-- Saves all parameters to EEPROM
void
Parameters::loadAllFromEeprom()
{
    uint32_t address = 0;
    for(int i = 0; i < ID_COUNT; i++) {
        uint8_t* ptr = (uint8_t*)mavParameters[i].value;
        for(int j = 0; j < mavParameters[i].length; j++, address++, ptr++) {
            *ptr = EEPROM.read(address);
        }
        #ifdef DEBUG
            Serial1.print("Loading from EEPROM: ");
            Serial1.print(mavParameters[i].id);
            Serial1.print(" Value: ");
            if(mavParameters[i].type == MAV_PARAM_TYPE_UINT32)
                Serial1.println(*((uint32_t*)mavParameters[i].value));
            else if(mavParameters[i].type == MAV_PARAM_TYPE_UINT16)
                Serial1.println(*((uint16_t*)mavParameters[i].value));
            else
                Serial1.println(*((int8_t*)mavParameters[i].value));
        #endif
    }
    #ifdef DEBUG
        Serial1.println("");
    #endif
    //-- Version is hardwired
    _sw_version = VERSION;
#ifdef ARDUINO_ARCH_ESP32
    _flash_left = ESP.getSketchSize();
#else
    _flash_left = ESP.getFreeSketchSpace();
#endif
}

//---------------------------------------------------------------------------------
//-- Compute parameters hash
uint32_t Parameters::paramHashCheck()
{
    uint32_t crc = 0;
    for(int i = 0; i < ID_COUNT; i++) {
        crc = _crc32part((uint8_t *)(void*)mavParameters[i].id, strlen(mavParameters[i].id), crc);
        //-- QGC Expects a CRC of sizeof(uint32_t)
        uint32_t val = 0;
        if(mavParameters[i].type == MAV_PARAM_TYPE_UINT32)
            val = *((uint32_t*)mavParameters[i].value);
        else if(mavParameters[i].type == MAV_PARAM_TYPE_UINT16)
            val = (uint32_t)*((uint16_t*)mavParameters[i].value);
        else
            val = (uint32_t)*((int8_t*)mavParameters[i].value);
        crc = _crc32part((uint8_t *)(void*)&val, sizeof(uint32_t), crc);
    }
    delay(0);
    return crc;
}

//---------------------------------------------------------------------------------
//-- Saves all parameters to EEPROM
void
Parameters::saveAllToEeprom()
{
    //-- Init flash space
    uint8_t* ptr = EEPROM.getDataPtr();
    memset(ptr, 0, EEPROM_SPACE);
    //-- Write all paramaters to flash
    uint32_t address = 0;
    for(int i = 0; i < ID_COUNT; i++) {
        ptr = (uint8_t*)mavParameters[i].value;
        #ifdef DEBUG
            Serial1.print("Saving to EEPROM: ");
            Serial1.print(mavParameters[i].id);
            Serial1.print(" Value: ");
            if(mavParameters[i].type == MAV_PARAM_TYPE_UINT32)
                Serial1.println(*((uint32_t*)mavParameters[i].value));
            else if(mavParameters[i].type == MAV_PARAM_TYPE_UINT16)
                Serial1.println(*((uint16_t*)mavParameters[i].value));
            else
                Serial1.println(*((int8_t*)mavParameters[i].value));
        #endif
        for(int j = 0; j < mavParameters[i].length; j++, address++, ptr++) {
            EEPROM.write(address, *ptr);
        }
    }
    uint32_t saved_crc = _getEepromCrc();
    EEPROM.put(EEPROM_CRC_ADD, saved_crc);
    EEPROM.commit();
    #ifdef DEBUG
        Serial1.print("Saved CRC: ");
        Serial1.print(saved_crc);
        Serial1.println("");
    #endif
}

//---------------------------------------------------------------------------------
//-- Compute CRC32 for a buffer
uint32_t
Parameters::_crc32part(uint8_t* src, uint32_t len, uint32_t crc)
{
    for (int i = 0;  i < (int)len;  i++) {
        crc = crc_table[(crc ^ src[i]) & 0xff] ^ (crc >> 8);
    }
    return crc;
}

//---------------------------------------------------------------------------------
//-- Computes EEPROM CRC
uint32_t
Parameters::_getEepromCrc()
{
    uint32_t crc  = 0;
    uint32_t size = 0;
    //-- Get size of all parameter data
    for(int i = 0; i < ID_COUNT; i++) {
        size += mavParameters[i].length;
    }
    //-- Computer CRC
    for (int i = 0 ; i < (int)size; i++) {
        crc = crc_table[(crc ^ EEPROM.read(i)) & 0xff] ^ (crc >> 8);
    }
    return crc;
}

//---------------------------------------------------------------------------------
//-- Initializes EEPROM. If not initialized, set to defaults and save it.
void
Parameters::_initEeprom()
{
    loadAllFromEeprom();
    //-- Is it uninitialized?
    uint32_t saved_crc = 0;
    EEPROM.get(EEPROM_CRC_ADD, saved_crc);
    uint32_t current_crc = _getEepromCrc();
    if(saved_crc != current_crc) {
        #ifdef DEBUG
            Serial1.print("Initializing EEPROM. Saved: ");
            Serial1.print(saved_crc);
            Serial1.print(" Current: ");
            Serial1.println(current_crc);
        #endif
        //-- Set all defaults
        resetToDefaults();
        //-- Save it all and store CRC
        saveAllToEeprom();
    } else {
        //-- Load all parameters from EEPROM
        loadAllFromEeprom();
    }
}

//---------------------------------------------------------------------------------
void
Parameters::setDebugEnabled(int8_t enabled)
{
    _debug_enabled     = enabled;
}

//---------------------------------------------------------------------------------
void
Parameters::setWifiMode(int8_t mode)
{
    _wifi_mode         = mode;
}

//---------------------------------------------------------------------------------
void
Parameters::setWifiChannel(uint32_t channel)
{
    _wifi_channel      = channel;
}

//---------------------------------------------------------------------------------
void
Parameters::setWifiUdpHport(uint16_t port)
{
    _wifi_udp_hport    = port;
}

//---------------------------------------------------------------------------------
void
Parameters::setWifiUdpCport(uint16_t port)
{
    _wifi_udp_cport    = port;
}

//---------------------------------------------------------------------------------
void
Parameters::setWifiSsid(const char* ssid)
{
    strncpy(_wifi_ssid, ssid, sizeof(_wifi_ssid));
}

//---------------------------------------------------------------------------------
void
Parameters::setWifiPassword(const char* pwd)
{
    strncpy(_wifi_password, pwd, sizeof(_wifi_password));
}

//---------------------------------------------------------------------------------
void
Parameters::setWifiStaSsid(const char* ssid)
{
    strncpy(_wifi_ssidsta, ssid, sizeof(_wifi_ssidsta));
}

//---------------------------------------------------------------------------------
void
Parameters::setWifiStaPassword(const char* pwd)
{
    strncpy(_wifi_passwordsta, pwd, sizeof(_wifi_passwordsta));
}

//---------------------------------------------------------------------------------
void
Parameters::setWifiStaIP(uint32_t addr)
{
    _wifi_ipsta = addr;
}

//---------------------------------------------------------------------------------
void
Parameters::setWifiStaGateway(uint32_t addr)
{
    _wifi_gatewaysta = addr;
}

//---------------------------------------------------------------------------------
void
Parameters::setWifiStaSubnet(uint32_t addr)
{
    _wifi_subnetsta = addr;
}

//---------------------------------------------------------------------------------
void
Parameters::setUartBaudRate(uint32_t baud)
{
    _uart_baud_rate = baud;
}

//---------------------------------------------------------------------------------
void
Parameters::setPWMServoChannel(uint8_t channel)
{
    // Clamp to valid range (5-16)
    if (channel < 5) {
        _pwm_servo_channel = 5;
    } else if (channel > 16) {
        _pwm_servo_channel = 16;
    } else {
        _pwm_servo_channel = channel;
    }
}
