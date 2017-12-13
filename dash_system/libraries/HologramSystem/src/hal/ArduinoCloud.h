/*
  ArduinoCloud.h - Class definitions that provide Arduino layer for Hologram
  Cloud on Dash

  https://hologram.io

  Copyright (c) 2017 Konekt, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#pragma once

#include "../sdk/cloud/Cloud.h"
#include "ArduinoUBlox.h"

typedef void (*event_callback)(ublox_event_id id, const ublox_event_content *content);

class ArduinoCloud : public Cloud, public NetworkEventHandler {
public:
    void begin(UBlox &ublox, AuthenticationMethod method, event_callback cb);
    virtual void onNetworkEvent(uint32_t id, const void* content);
    virtual void onPowerUp();
    virtual void onPowerDown();

    static uint32_t getSecondsUTC(const timestamp_tz &ts);
    static bool convertToUTC(rtc_datetime_t &dest, const timestamp_tz &source);
    bool convertToLocalTime(rtc_datetime_t &dest, const timestamp_tz &source);

    bool checkOTA(int index, sms_event &ota_sms);

protected:
    virtual const char* getID();
    virtual const char* getKey();
    virtual const char* getHost();
    virtual int getPort();
    virtual uint32_t getSeconds();

    bool downloadOTA(const char* url, const char* destination);
    bool programOTA(const char* filename);
    int findLine(const char* filename, int offset, const char* linestart, char* buffer, int size);
    bool findContent(const char* filename, int* contentLength, int* contentOffset);
    int findBlankLine(const char* filename, int offset, char* buffer, int size);

    char imsi[20];
    char iccid[20];

    event_callback event_cb;
    UBlox *ublox;
    sms_event sms;
};

extern ArduinoCloud Cloud;
