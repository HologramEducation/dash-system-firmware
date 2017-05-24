/*
  SPIFlash.cpp - Class definitions that provide a Flash subclass for a SPI Flash
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
#include "SPIFlash.h"
#include "wiring_digital.h"

SPIFlash::SPIFlash(uint32_t sectorSize, uint32_t maxWrite, uint32_t ssPin)
:Flash(sectorSize, maxWrite), spi(NULL), ssPin(ssPin){}

void SPIFlash::init(Spi &spi)
{
    this->spi = &spi;
}

bool SPIFlash::ready()
{
    return begun && (spi != NULL);
}

void SPIFlash::begin()
{
    begun = true;
    pinMode(ssPin, OUTPUT);
    digitalWrite(ssPin, HIGH);
}

uint8_t SPIFlash::pollBusy()
{
    if(!ready()) return 0xFF;
    uint8_t status = 0;
    do{
        status = readStatus();
    }while(status & 0x01);
    return status;
}

void SPIFlash::enableWrites(bool enable)
{
    if(!ready()) return;
    digitalWrite(ssPin, LOW);
    spi->transfer(enable ? 0x06 : 0x04);
    digitalWrite(ssPin, HIGH);
}

uint8_t SPIFlash::readStatus()
{
    if(!ready()) return 0xFF;
    digitalWrite(ssPin, LOW);
    spi->transfer(0x05);
    uint8_t status = spi->transfer();
    digitalWrite(ssPin, HIGH);
    return status;
}

void SPIFlash::read(uint32_t address, uint8_t *buffer, size_t count)
{
    if(!ready()) return;
    digitalWrite(ssPin, LOW);
    spi->transfer(0x03);
    spi->transfer((address >> 16) & 0xFF);
    spi->transfer((address >> 8) & 0xFF);
    spi->transfer(address & 0xFF);
    for(int i=0; i<count; i++)
        buffer[i] = spi->transfer();
    digitalWrite(ssPin, HIGH);
}

uint8_t SPIFlash::write(uint32_t address, uint8_t *buffer, size_t count)
{
    if(!ready()) return 0xFF;
    uint32_t offset = 0;
    uint8_t status = 0;
    while(count)
    {
        size_t towrite = count;
        //find next boundary
        uint32_t mask = maxWrite-1;
        uint32_t next_boundary = address | mask;
        if(address + towrite > next_boundary)
            towrite = next_boundary - address + 1;
        count -= towrite;

        enableWrites(true);
        digitalWrite(ssPin, LOW);
        spi->transfer(0x02);
        spi->transfer((address >> 16) & 0xFF);
        spi->transfer((address >> 8) & 0xFF);
        spi->transfer(address & 0xFF);
        for(int i=0; i<towrite; i++)
            spi->transfer(buffer[i+offset]);
        digitalWrite(ssPin, HIGH);
        status = pollBusy();
        enableWrites(false);
        address += towrite;
        offset += towrite;
    }
    return status;
}

uint8_t SPIFlash::command(uint32_t address, uint8_t command, bool addressed)
{
    if(!ready()) return 0xFF;
    enableWrites(true);
    digitalWrite(ssPin, LOW);
    spi->transfer(command);
    if(addressed)
    {
        spi->transfer((address >> 16) & 0xFF);
        spi->transfer((address >> 8) & 0xFF);
        spi->transfer(address & 0xFF);
    }
    digitalWrite(ssPin, HIGH);
    uint8_t status = pollBusy();
    enableWrites(false);
    return status;
}

void SPIFlash::beginRead(uint32_t address)
{
    if(!ready()) return;
    digitalWrite(ssPin, LOW);
    spi->transfer(0x03);
    spi->transfer((address >> 16) & 0xFF);
    spi->transfer((address >> 8) & 0xFF);
    spi->transfer(address & 0xFF);
}

uint8_t SPIFlash::continueRead()
{
    return spi->transfer();
}

void SPIFlash::endRead()
{
    digitalWrite(ssPin, HIGH);
}

void SPIFlash::beginWrite(uint32_t address)
{
    if(!ready()) return;
    enableWrites(true);
    digitalWrite(ssPin, LOW);
    spi->transfer(0x02);
    spi->transfer((address >> 16) & 0xFF);
    spi->transfer((address >> 8) & 0xFF);
    spi->transfer(address & 0xFF);
}

void SPIFlash::continueWrite(uint8_t byte)
{
    spi->transfer(byte);
}

void SPIFlash::endWrite()
{
    digitalWrite(ssPin, HIGH);
    pollBusy();
    enableWrites(false);
}
