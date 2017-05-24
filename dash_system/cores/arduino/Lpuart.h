/*
  Lpuart.h - Class definitions that provide a Stream subclass of a Low Power
  UART peripheral.

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

#include "RingBuffer.h"
#include "Stream.h"
#include "hal/fsl_device_registers.h"

#include <cstddef>

class Lpuart : public Stream
{
public:
    Lpuart(LPUART_Type * instance, sim_clock_gate_name_t gate_name, uint32_t clock,
        IRQn_Type irqNumber, uint32_t rx, uint32_t tx);
    void begin(unsigned long baudRate); //(8N1 only) TODO add config params

    void flowcontrol(bool enable, uint32_t rts, uint32_t cts);
    bool flowcontrol();
    void pause();
    void resume();
    bool paused();

    void flush();
    void IrqHandler();
    size_t write(const uint8_t data);
    void end();
    int available();
    int peek();
    int read();
    operator bool() { return true; }
    using Print::write; // pull in write(str) and write(buf, size) from Print

    void waitToEmpty();

protected:
    RingBuffer rxBuffer;
    LPUART_Type * instance;
    sim_clock_gate_name_t gate_name;
    uint32_t clock;
    IRQn_Type irqNumber;
    uint32_t rx;
    uint32_t tx;
    uint32_t rts;
    uint32_t cts;
    bool use_flowcontrol;
};
