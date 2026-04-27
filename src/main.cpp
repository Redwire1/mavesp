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
 * @file main.cpp
 * ESP8266/ESP32 Wifi AP, MavLink UART/UDP Bridge
 *
 * @author Gus Grubba <mavlink@grubba.com>
 */

#include "bridge.h"
#include "parameters.h"
#include "gcs.h"
#include "vehicle.h"
#include "httpd.h"
#include "component.h"
#include "ppm.h"

#ifdef ARDUINO_ARCH_ESP32
    #include <esp_ota_ops.h>
#endif

#ifdef ARDUINO_ARCH_ESP32
    #include <ESPmDNS.h>
#else
    #include <ESP8266mDNS.h>
#endif

#define GPIO02  2

//---------------------------------------------------------------------------------
//-- HTTP Update Status
class OtaUpdateImp : public OtaUpdate {
public:
    OtaUpdateImp ()
        : _isUpdating(false)
    {

    }
    void updateStarted  ()
    {
        _isUpdating = true;
    }
    void updateCompleted()
    {
        //-- TODO
    }
    void updateError    ()
    {
        //-- TODO
    }
    bool isUpdating     () { return _isUpdating; }
private:
    bool _isUpdating;
};



// T023: Track whether the new firmware has been confirmed valid yet.
// Set to true on the first HTTP request handled after boot; at that point
// esp_ota_mark_app_valid_cancel_rollback() is called so the ESP-IDF OTA
// watchdog doesn't roll back to the previous image on the next power cycle.
static bool _ota_confirmed = false;

//-- Singletons
IPAddress               localIP;
Component               gComponent;
Parameters              gParameters;
Gcs                     gGCS;
Vehicle                 gVehicle;
Httpd                   updateServer;
OtaUpdateImp            updateStatus;
Log                     gLogger;
Ppm                     gPWMConverter;

//---------------------------------------------------------------------------------
//-- Accessors
class WorldImp : public World {
public:
    Parameters*   getParameters   () { return &gParameters;    }
    Component*    getComponent    () { return &gComponent;     }
    Vehicle*      getVehicle      () { return &gVehicle;       }
    Gcs*          getGCS          () { return &gGCS;           }
    Log*          getLogger       () { return &gLogger;        }
    Ppm*          getPWM          () { return &gPWMConverter;  }
};

WorldImp                gWorld;

World* getWorld()
{
    return &gWorld;
}

//---------------------------------------------------------------------------------
//-- Wait for a DHCPD client
void wait_for_client() {
    DEBUG_LOG("Waiting for a client...\n");
#ifdef ENABLE_DEBUG
    int wcount = 0;
#endif
#ifdef ARDUINO_ARCH_ESP32
    uint8_t client_count = WiFi.softAPgetStationNum();
#else
    uint8 client_count = wifi_softap_get_station_num();
#endif
    while (!client_count) {
#ifdef ENABLE_DEBUG
        Serial1.print(".");
        if(++wcount > 80) {
            wcount = 0;
            Serial1.println();
        }
#endif
        delay(1000);
#ifdef ARDUINO_ARCH_ESP32
        client_count = WiFi.softAPgetStationNum();
#else
        client_count = wifi_softap_get_station_num();
#endif
    }
    DEBUG_LOG("Got %d client(s)\n", client_count);
}

//---------------------------------------------------------------------------------
//-- Reset all parameters whenever the reset gpio pin is active
void reset_interrupt(){
    gParameters.resetToDefaults();
    gParameters.saveAllToEeprom();
#ifdef ARDUINO_ARCH_ESP32
    ESP.restart();
#else
    ESP.reset();
#endif
}

//---------------------------------------------------------------------------------
//-- Set things up
void setup() {
    delay(1000);
    gParameters.begin();
#ifdef ENABLE_DEBUG
    //   We only use it for non debug because GPIO02 is used as a serial
    //   pin (TX) when debugging.
    Serial1.begin(115200);
#else
    //-- Initialized GPIO02 (Used for "Reset To Factory")
    pinMode(GPIO02, INPUT_PULLUP);
    attachInterrupt(GPIO02, reset_interrupt, FALLING);
#endif
    gLogger.begin(2048);

    DEBUG_LOG("\nConfiguring access point...\n");
#ifdef ARDUINO_ARCH_ESP32
    DEBUG_LOG("Free Heap: %u\n", ESP.getFreeHeap());
#else
    DEBUG_LOG("Free Sketch Space: %u\n", ESP.getFreeSketchSpace());
#endif

    WiFi.disconnect(true);

    if(gParameters.getWifiMode() == WIFI_PREF_STA){
        //-- Connect to an existing network
        WiFi.mode(WIFI_STA);
        WiFi.config(gParameters.getWifiStaIP(), gParameters.getWifiStaGateway(), gParameters.getWifiStaSubnet(), 0U, 0U);
        WiFi.begin(gParameters.getWifiStaSsid(), gParameters.getWifiStaPassword());

        //-- Wait a minute to connect
        for(int i = 0; i < 120 && WiFi.status() != WL_CONNECTED; i++) {
            #ifdef ENABLE_DEBUG
            Serial.print(".");
            #endif
            delay(500);
        }
        if(WiFi.status() == WL_CONNECTED) {
            localIP = WiFi.localIP();
            WiFi.setAutoReconnect(true);
        } else {
            //-- Fall back to AP mode if no connection could be established
            WiFi.disconnect(true);
            gParameters.setWifiMode(WIFI_PREF_AP);
        }
    }

    if(gParameters.getWifiMode() == WIFI_PREF_AP){
        //-- Start AP
        WiFi.mode(WIFI_AP);
#ifdef ARDUINO_ARCH_ESP32
        WiFi.softAP(gParameters.getWifiSsid(), gParameters.getWifiPassword(), gParameters.getWifiChannel());
#else
        WiFi.encryptionType(AUTH_WPA2_PSK);
        WiFi.softAP(gParameters.getWifiSsid(), gParameters.getWifiPassword(), gParameters.getWifiChannel());
#endif
        localIP = WiFi.softAPIP();
        wait_for_client();
    }

#ifdef ARDUINO_ARCH_ESP32
    //-- Boost power to Max (ESP32)
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
#else
    //-- Boost power to Max (ESP8266)
    WiFi.setOutputPower(20.5);
#endif
    //-- MDNS
    char mdsnName[256];
    sprintf(mdsnName, "MavEsp8266-%d",localIP[3]);
    MDNS.begin(mdsnName);
    MDNS.addService("http", "tcp", 80);
    //-- Initialize Comm Links
    DEBUG_LOG("Start WiFi Bridge\n");
    DEBUG_LOG("Local IP: %s\n", localIP.toString().c_str());

    gParameters.setLocalIPAddress(localIP);
    IPAddress gcs_ip(localIP);
    //-- I'm getting bogus IP from the DHCP server. Broadcasting for now.
    gcs_ip[3] = 255;
    gGCS.begin(&gVehicle, gcs_ip);
    gVehicle.begin(&gGCS);
    //-- Initialize PWM Converter (ESP32 only) - Now using polling (WiFi-safe)
#ifdef ARDUINO_ARCH_ESP32
    gPWMConverter.begin();
#endif
    //-- Initialize Update Server
    updateServer.begin(&updateStatus);
}

//---------------------------------------------------------------------------------
//-- Main Loop
void loop() {
    if(!updateStatus.isUpdating()) {
        if (gComponent.inRawMode()) {
            gGCS.readMessageRaw();
            delay(0);
            gVehicle.readMessageRaw();

        } else {
            gGCS.readMessage();
            delay(0);
            gVehicle.readMessage();
        }
    }
#ifdef ARDUINO_ARCH_ESP32
    gPWMConverter.update();  // Now on GPIO14 (no conflict with GPIO2 factory reset)
#endif
    updateServer.checkUpdates();
    // T023: Confirm firmware valid after first HTTP request (cancels OTA rollback)
#ifdef ARDUINO_ARCH_ESP32
    if (!_ota_confirmed) {
        _ota_confirmed = true;
        esp_ota_mark_app_valid_cancel_rollback();
    }
#endif
}
