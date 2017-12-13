/*
  ArduinoModem.h - Class definitions that provide Arduino layer for a ublox
  interface on the Dash.

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

#include "../sdk/network/ublox/UBlox.h"
#include "ArduinoModem.h"
#include "Arduino.h"

class ArduinoUBlox : public UBlox {
public:
    void begin(NetworkEventHandler &h, Stream &modem_uart, Stream *uart=NULL);
protected:
    virtual void wait(uint32_t ms);
    virtual void holdReset();
    virtual void releaseReset();
    virtual void toggleReset();
    virtual void debug(const char* msg);
    virtual void debugln(const char* msg);
    virtual void debug(int i);
    virtual void debugln(int i);
    virtual uint32_t modemResetTime() { return DASH_1_2 ? 6 : 4;}

    Stream *uart;
    ArduinoModem modem;
};

extern ArduinoUBlox ublox;
