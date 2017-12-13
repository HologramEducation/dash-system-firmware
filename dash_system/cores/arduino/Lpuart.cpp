/*
  Lpuart.cpp - Class definitions that provide a Stream subclass of a Low Power
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
#include "Lpuart.h"
#include "Arduino.h"
#include "hal/fsl_lpuart_hal.h"

Lpuart::Lpuart(LPUART_Type * instance, sim_clock_gate_name_t gate_name, uint32_t clock,
    IRQn_Type irqNumber, uint32_t rx, uint32_t tx)
: instance(instance), gate_name(gate_name), clock(clock), irqNumber(irqNumber),
  rx(rx), tx(tx), use_flowcontrol(false)
{
}

void Lpuart::end()
{
    LPUART_HAL_Init(instance);
    NVIC_DisableIRQ(irqNumber);
    pinMode(rx, DISABLE);
    pinMode(tx, DISABLE);
    SIM_HAL_DisableClock(SIM, gate_name);
    rxBuffer.clear();
    if(use_flowcontrol)
        digitalWrite(rts, HIGH);
}

int Lpuart::available()
{
    return rxBuffer.available();
}

int Lpuart::peek()
{
    return rxBuffer.peek();
}

int Lpuart::read()
{
    return rxBuffer.read_char();
}

void Lpuart::begin(uint32_t baudrate)
{
    SIM_HAL_EnableClock(SIM, gate_name);

    PORT_CLOCK_ENABLE(rx);
    PORT_CLOCK_ENABLE(tx);
    PORT_SET_MUX_UART(rx);
    PORT_SET_MUX_UART(tx);

    LPUART_HAL_Init(instance);

    LPUART_HAL_SetBaudRate(instance, SystemClockLookup(clock), baudrate);
    LPUART_HAL_SetBitCountPerChar(instance, kLpuart8BitsPerChar);
    LPUART_HAL_SetParityMode(instance, kLpuartParityDisabled);
    LPUART_HAL_SetStopBitCount(instance, kLpuartOneStopBit);

    LPUART_HAL_SetIntMode(instance, kLpuartIntRxDataRegFull, true);
    NVIC_EnableIRQ(irqNumber);

    LPUART_HAL_SetTransmitterCmd(instance, true);
    LPUART_HAL_SetReceiverCmd(instance, true);
}

void Lpuart::flowcontrol(bool enable, uint32_t rts, uint32_t cts)
{
    use_flowcontrol = enable;
    this->rts = rts;
    this->cts = cts;
    if(enable)
    {
        pinMode(cts, INPUT_PULLUP);
        pinMode(rts, OUTPUT);
        digitalWrite(rts, LOW);
    }
    else
    {
        pinMode(cts, INPUT);
        pinMode(rts, INPUT);
    }
}

bool Lpuart::flowcontrol()
{
    return use_flowcontrol;
}

void Lpuart::pause()
{
    if(use_flowcontrol)
        digitalWrite(rts, HIGH);
}

void Lpuart::resume()
{
    if(use_flowcontrol)
        digitalWrite(rts, LOW);
}

bool Lpuart::paused()
{
    digitalRead(rts) == HIGH;
}

void Lpuart::flush()
{
    rxBuffer.clear();
}

void Lpuart::waitToEmpty()
{
    if(!SIM_HAL_GetGateCmd(SIM, gate_name)) return;
    uint32_t start = millis();

    while(!LPUART_BRD_STAT_TDRE(instance))
    {
        if(millis() - start > 10)
            break;
    }

    while(!LPUART_BRD_STAT_TC(instance))
    {
        if(millis() - start > 10)
            break;
    }
}

void Lpuart::IrqHandler()
{
    while(LPUART_RD_STAT_RDRF(instance))
        rxBuffer.store_char(LPUART_RD_DATA(instance));
}

size_t Lpuart::write(const uint8_t data)
{
    if(!SIM_HAL_GetGateCmd(SIM, gate_name)) return 0;
    uint32_t start = millis();

    while (!LPUART_BRD_STAT_TDRE(instance))
    {
        if(millis() - start > 10)
            return 0;
    }

    if(use_flowcontrol)
    {
        //implement timeout?
        while(digitalRead(cts) == HIGH);
    }

    LPUART_HAL_Putchar(instance, data);
    return 1;
}
