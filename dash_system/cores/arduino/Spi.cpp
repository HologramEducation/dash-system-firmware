/*
  Spi.cpp - Class definitions that provide access to the SPI peripheral on a
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
#include "Spi.h"
#include "Arduino.h"

SPISettings::SPISettings(uint32_t clock, BitOrder bitOrder, uint8_t dataMode)
: clockFreq(clock)
{
    direction = (bitOrder == MSBFIRST) ? kSpiMsbFirst : kSpiLsbFirst;
    phase = (spi_clock_phase_t)(dataMode & 1);
    polarity = (spi_clock_polarity_t)((dataMode >> 1) & 1);
}

Spi::Spi(SPI_Type * instance, sim_clock_gate_name_t gate_name, uint32_t clock,
    IRQn_Type irqNumber, uint32_t fifo_size, uint32_t miso, uint32_t mosi,
    uint32_t sclk)
{
    this->instance = instance;
    this->gate_name = gate_name;
    this->clock = clock;
    this->irqNumber = irqNumber;
    this->fifo_size = fifo_size;
    this->miso = miso;
    this->mosi = mosi;
    this->sclk = sclk;
}

void Spi::begin()
{
    SIM_HAL_EnableClock(SIM, gate_name);

    PORT_CLOCK_ENABLE(miso);
    PORT_CLOCK_ENABLE(mosi);
    PORT_CLOCK_ENABLE(sclk);
    PORT_SET_MUX_SPI(miso);
    PORT_SET_MUX_SPI(mosi);
    PORT_SET_MUX_SPI(sclk);

    SPI_HAL_Init(instance);
    SPI_HAL_SetMasterSlave(instance, kSpiMaster);
    SPI_HAL_SetSlaveSelectOutputMode(instance, kSpiSlaveSelect_AsGpio);
    SPI_HAL_SetPinMode(instance, kSpiPinMode_Normal);

    SPI_BWR_C2_MODFEN(instance, 0);

    //TODO: Only call this for devices that have this register. SPI0 does not.
    //SPI_BWR_C3_FIFOMODE(instance, 0);


    // if (fifo_size)
    //     SPI_HAL_SetFifoMode(instance, true, kSpiTxFifoOneHalfEmpty, kSpiRxFifoOneHalfFull);

    //NVIC_EnableIRQ(irqNumber);
    //SPI_HAL_Enable(instance);
    beginTransaction();
}

void Spi::end()
{
    SPI_HAL_Init(instance);
    //NVIC_DisableIRQ(irqNumber);
    pinMode(miso, DISABLE);
    pinMode(mosi, DISABLE);
    pinMode(sclk, DISABLE);
    SIM_HAL_DisableClock(SIM, gate_name);
}

uint32_t Spi::beginTransaction(SPISettings settings)
{
    SPI_HAL_Disable(instance);
    uint32_t actual = SPI_HAL_SetBaud(instance, settings.clockFreq, SystemClockLookup(clock));
    SPI_HAL_SetDataFormat(instance, settings.polarity, settings.phase, settings.direction);
    SPI_HAL_Enable(instance);
    //while((SPI_RD_S_SPTEF(instance) == 0));
    return actual;
}

uint32_t Spi::beginTransaction()
{
    return beginTransaction(SPISettings());
}

void Spi::endTransaction(void)
{

}

uint8_t Spi::transfer(uint8_t data)
{
    // if(SPI_RD_S_SPTEF(instance) == 0)
    //     return 0xFF;
    while((SPI_RD_S_SPTEF(instance) == 0));
    SPI_WR_DL(instance, data);
    //uint32_t count = 100;
	//while(SPI_RD_S_SPRF(instance) == 0 && count--);
    while(SPI_RD_S_SPRF(instance) == 0);
	return SPI_RD_DL(instance);
}
