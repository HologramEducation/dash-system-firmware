/*
  Spi.h - Class definitions that provide access to the SPI peripheral on a
  Kinetis MCU.

  https://hologram.io

  Copyright (c) 2017 Konekt, Inc.  All rights reserved.

  Derived from file with original copyright notice:
  Arduino.h - Main include file for the Arduino SDK
  Copyright (c) 2014 Arduino LLC.  All right reserved.

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

#include "hal/fsl_device_registers.h"
#include "hal/fsl_spi_hal.h"
#include "wiring_constants.h"

class SPISettings {
public:
    SPISettings(uint32_t clock, BitOrder bitOrder, uint8_t mode);
    SPISettings() : SPISettings(1000000, MSBFIRST, 0){}
    uint32_t clockFreq;
    spi_clock_polarity_t polarity;
    spi_clock_phase_t phase;
    spi_shift_direction_t direction;

    friend class Spi;
};

class Spi {
public:
    Spi(SPI_Type * instance, sim_clock_gate_name_t gate_name, uint32_t clock,
        IRQn_Type irqNumber, uint32_t fifo_size, uint32_t miso, uint32_t mosi,
        uint32_t sclk);

    uint8_t transfer(uint8_t data=0);
    inline void transfer(void *buf, size_t count);
    //
    // // Transaction Functions
    // void usingInterrupt(int interruptNumber);
    uint32_t beginTransaction();
    uint32_t beginTransaction(SPISettings settings);
    void endTransaction(void);
    //
    // // SPI Configuration methods
    // void attachInterrupt();
    // void detachInterrupt();
    //
    void begin();
    void end();
    //
    // void setBitOrder(BitOrder order);
    // void setDataMode(uint8_t uc_mode);
    // void setClockDivider(uint8_t uc_div);

private:
    // void init();
    // void config(SPISettings settings);

    SPI_Type * instance;
    sim_clock_gate_name_t gate_name;
    uint32_t clock;
    IRQn_Type irqNumber;
    uint32_t fifo_size;
    uint32_t miso;
    uint32_t mosi;
    uint32_t sclk;
    // bool initialized;
    // uint8_t interruptMode;
    // char interruptSave;
    // uint32_t interruptMask;
};

void Spi::transfer(void *buf, size_t count)
{
    uint8_t *buffer = reinterpret_cast<uint8_t *>(buf);
    for (size_t i=0; i<count; i++)
        buffer[i] = transfer(buffer[i]);
}
