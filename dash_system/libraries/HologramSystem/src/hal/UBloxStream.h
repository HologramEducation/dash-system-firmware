/*
  UBloxStream.h - Class definitions that provide Arduino Stream compatibility
  for ublox flash memory on the Dash.

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

#include "Stream.h"
#include "ArduinoUBlox.h"

class UBloxStream : public Stream
{
public:
    UBloxStream(UBlox *u, const char* filename, unsigned int offset, unsigned int bytes);
    virtual int available();
    virtual int read();
    virtual int peek();
    virtual void flush(){}
    virtual size_t write(uint8_t){return 0;}
protected:
    #define UBLOXSTREAM_BUFFER_SIZE 32
    UBlox *reader;
    int fill_loc;
    unsigned int buff_loc;
    const char * filename;
    int offset;
    unsigned int bytes;
    unsigned int buffered;
    char buffer[UBLOXSTREAM_BUFFER_SIZE];
};
