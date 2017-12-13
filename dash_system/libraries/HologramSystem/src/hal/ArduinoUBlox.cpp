/*
  ArduinoModem.cpp - Class definitions that provide Arduino layer for a ublox
  interface on the Dash.

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
#include "ArduinoUBlox.h"

void ArduinoUBlox::begin(NetworkEventHandler &h, Stream &modem_uart, Stream *uart) {
    modem.begin(modem_uart, *this);
    this->uart = uart;
    init(h, modem);
}

void ArduinoUBlox::wait(uint32_t ms) {
    delay(ms);
}

void ArduinoUBlox::holdReset() {
    System.ubloxReset(true);
}

void ArduinoUBlox::releaseReset() {
    System.ubloxReset(false);
}

void ArduinoUBlox::toggleReset() {
    System.ubloxReset();
}

void ArduinoUBlox::debug(const char* msg) {
    if(uart)
        uart->print(msg);
}

void ArduinoUBlox::debugln(const char* msg) {
    if(uart)
        uart->println(msg);
}

void ArduinoUBlox::debug(int i) {
    if(uart)
        uart->print(i);
}

void ArduinoUBlox::debugln(int i) {
    if(uart)
        uart->println(i);
}

ArduinoUBlox ublox;
