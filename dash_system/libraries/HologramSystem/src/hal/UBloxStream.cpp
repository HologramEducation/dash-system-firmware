/*
  UBloxStream.cpp - Class definitions that provide Arduino Stream compatibility
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

#include "UbloxStream.h"
#include "Arduino.h"

UBloxStream::UBloxStream(UBlox *u, const char * filename, unsigned int offset, unsigned int bytes)
: reader(u), filename(filename), offset(offset), bytes(bytes), fill_loc(0), buff_loc(0), buffered(0){}

int UBloxStream::available()
{
    return (bytes - fill_loc) + (buffered - buff_loc);
}

int UBloxStream::read()
{
    if(!available()) return 0;
    if(buff_loc >= buffered)
    {
        buffered = reader->readFile(filename, offset + fill_loc, buffer,
                                    min(bytes - fill_loc, UBLOXSTREAM_BUFFER_SIZE));
        if(buffered == 0) return 0;
        buff_loc = 0;
        fill_loc += buffered;
    }

    return buffer[buff_loc++];
}

int UBloxStream::peek()
{
    int r = read();
    --buff_loc;
    return r;
}
