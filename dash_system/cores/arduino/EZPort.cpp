/*
  EZPort.cpp - Class definitions that provide access to the
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

#include "EZPort.h"
#include "wiring_digital.h"
#include "delay.h"

EZPort::EZPort(uint32_t sectorSize, uint32_t maxWrite, uint32_t ssPin, uint32_t resetPin)
:SPIFlash(sectorSize, maxWrite, ssPin), resetPin(resetPin){}

void EZPort::begin()
{
    if(spi == NULL) return;

    spi->begin();
    pinMode(ssPin, OUTPUT);
    digitalWrite(ssPin, LOW);
    pinMode(resetPin, OUTPUT);
    digitalWrite(resetPin, LOW);
    delay(10);
    digitalWrite(resetPin, HIGH);
    delay(10);
    digitalWrite(ssPin, HIGH);
    begun = true;
}

void EZPort::end()
{
    begun = false;
    spi->end();
    pinMode(ssPin, INPUT);
    pinMode(resetPin, OUTPUT);
    digitalWrite(resetPin, LOW);
    delay(10);
    pinMode(resetPin, INPUT);
    delay(10);
    pinMode(ssPin, DISABLE);
    pinMode(resetPin, DISABLE);
}
