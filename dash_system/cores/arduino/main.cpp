/*
  main.cpp - entry point for Arduino runtime

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

#define ARDUINO_MAIN
#include "Arduino.h"

void initVariant(void);

int main( void )
{
    init();

    initVariant();

    delay(1);
#if defined(USBCON)
    USBDevice.init();
    USBDevice.attach();
#endif

    setup();

    for (;;)
    {
        loop();
        //if (serialEventRun) serialEventRun();
    }

    return 0;
}
