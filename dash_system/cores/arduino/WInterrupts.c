/*
  WInterrupts.c - Interrupt handling for Hologram Dash

  https://hologram.io

  Copyright (c) 2017 Konekt, Inc.  All rights reserved.


  Derived from file with original copyright notice:

  Copyright (c) 2011-2012 Arduino.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "WInterrupts.h"

typedef void (*interruptCB)(void);

static interruptCB callbacks[INTERRUPT_PIN_COUNT];

uint32_t digitalPinToInterrupt(uint32_t pin)
{
    return pin;
}

void attachInterrupt(uint32_t pin, void (*callback)(void), uint32_t mode)
{
    uint32_t config = 0;
    if(pin >= INTERRUPT_PIN_COUNT) return;
    if(IO_NOT_DIGITAL(pin)) return;
    PORT_CLOCK_ENABLE(pin);

    switch(mode)
    {
        case LOW:     config = 0x8; break;
        case HIGH:    config = 0xC; break;
        default:
        case CHANGE:  config = 0xB; break;
        case FALLING: config = 0xA; break;
        case RISING:  config = 0x9; break;
    }

    callbacks[pin] = callback;
    PORT_WR_PCR_ISF(PERIPH_PORT(pin), PINS_PIN(pin), 1);
    PORT_WR_PCR_IRQC(PERIPH_PORT(pin), PINS_PIN(pin), config);
    if(PINS_PORT(pin) == PORT_A)
        NVIC_EnableIRQ((IRQn_Type)PORTA_IRQn);
    else
        NVIC_EnableIRQ((IRQn_Type)PORTCD_IRQn);
}

void detachInterrupt(uint32_t pin)
{
    if(pin >= INTERRUPT_PIN_COUNT) return;
    if(IO_NOT_DIGITAL(pin)) return;
    PORT_WR_PCR_IRQC(PERIPH_PORT(pin), PINS_PIN(pin), 0);
    PORT_WR_PCR_ISF(PERIPH_PORT(pin), PINS_PIN(pin), 1);
}

void handleInterrupt(uint32_t port)
{
    if(port > 3) return;

    uint32_t flags = PORT_RD_ISFR(PERIPH_FROM_PORT(port));
    PORT_WR_ISFR(PERIPH_FROM_PORT(port), ~0U);

    if(flags)
    {
        for(int i=0; i<INTERRUPT_PIN_COUNT; i++)
        {
            if(PINS_PORT(i) != port) continue;

            if(flags & (1<<(PINS_PIN(i))))
            {
                if(callbacks[i])
                    callbacks[i]();
            }
        }
    }
}
