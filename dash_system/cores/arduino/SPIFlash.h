/*
  SPIFlash.h - Class definitions that provide a Flash subclass for a SPI Flash
  memory.

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

#include "Flash.h"
#include "Spi.h"
#include "WString.h"
#include "Stream.h"

class SPIFlash : public Flash{
public:
    SPIFlash(uint32_t sectorSize, uint32_t maxWrite, uint32_t ssPin);
    void init(Spi &spi);
    virtual bool ready();
    virtual void begin();
    virtual void read(uint32_t address, uint8_t *buffer, size_t count);
    virtual uint8_t write(uint32_t address, uint8_t *buffer, size_t count);
    virtual uint8_t eraseSector(uint32_t address) {return command(address, 0x20);}
    virtual uint8_t eraseAll() {return command(0, 0xC7, false);}

    virtual void beginRead(uint32_t address);
    virtual uint8_t continueRead() override;
    virtual void endRead();
    virtual void beginWrite(uint32_t address);
    virtual void continueWrite(uint8_t byte);
    virtual void endWrite();

    virtual uint8_t readStatus();

protected:
    Spi *spi;
    uint32_t ssPin;

    void enableWrites(bool enable);
    uint8_t pollBusy();
    uint8_t command(uint32_t address, uint8_t command, bool addressed=true);
};
