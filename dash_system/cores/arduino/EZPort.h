/*
  EZPort.h - Class definitions that provide access to the
  Kinetis EZPort SPI flash emulation peripheral

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

#include "SPIFlash.h"

class EZPort : public SPIFlash{
public:
    EZPort(uint32_t sectorSize, uint32_t maxWrite, uint32_t ssPin, uint32_t resetPin);
    void begin();
    void end();
    uint8_t eraseSector(uint32_t address) {return command(address, 0xD8);}

protected:
    uint32_t resetPin;
};
