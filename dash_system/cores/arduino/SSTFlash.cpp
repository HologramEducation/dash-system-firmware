/*
  SSTFlash.cpp - Class definitions that provide a SPIFlash subclass for an SST
  brand SPI Flash memory.

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
#include "SSTFlash.h"
#include "wiring_digital.h"
#include "delay.h"

SSTFlash::SSTFlash(uint32_t sectorSize, uint32_t maxWrite, uint32_t ssPin)
:SPIFlash(sectorSize, maxWrite, ssPin){}

void SSTFlash::begin()
{
    if(spi == NULL) return;

    pinMode(ssPin, OUTPUT);
    digitalWrite(ssPin, HIGH);

    //release from power-down mode
    delayMicroseconds(10);
    digitalWrite(ssPin, LOW);
    spi->transfer(0xAB);
    digitalWrite(ssPin, HIGH);
    delayMicroseconds(10);

    begun = true;
    reset();
}

void SSTFlash::end()
{
    if(spi == NULL) return;
    if(!begun)
    {
        pinMode(ssPin, OUTPUT);
        digitalWrite(ssPin, HIGH);
        delayMicroseconds(10);
    }

    //power-down mode
    digitalWrite(ssPin, LOW);
    spi->transfer(0xB9);
    digitalWrite(ssPin, HIGH);
    begun = false;
}

uint32_t SSTFlash::id()
{
    if(!ready()) return 0xFFFFFFFF;
    uint32_t val = 0;
    digitalWrite(ssPin, LOW);
    spi->transfer(0x9F);
    val = spi->transfer() << 16;
    val |= spi->transfer() << 8;
    val |= spi->transfer();
    digitalWrite(ssPin, HIGH);
    return val;
}

uint8_t SSTFlash::readConfig()
{
    if(!ready()) return 0xFF;
    digitalWrite(ssPin, LOW);
    spi->transfer(0x35);
    uint8_t config = spi->transfer();
    digitalWrite(ssPin, HIGH);
    return config;
}

void SSTFlash::readBlockProtect(uint8_t *buffer)
{
    if(!ready()) return;
    digitalWrite(ssPin, LOW);
    spi->transfer(0x72);
    for(int i=0; i<6; i++)
        buffer[i] = spi->transfer();
    digitalWrite(ssPin, HIGH);
}

void SSTFlash::reset()
{
    if(!ready()) return;
    digitalWrite(ssPin, LOW);
    spi->transfer(0x66);
    digitalWrite(ssPin, HIGH);
    digitalWrite(ssPin, LOW);
    spi->transfer(0x99);
    digitalWrite(ssPin, HIGH);
}
