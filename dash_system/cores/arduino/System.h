/*
  System.h - Class definitions that provide special
  UI and features specific for the Hologram Dash family of products.

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

#include "hal/fsl_device_registers.h"
#include "wiring_digital.h"
#include "wiring_constants.h"
#include "WString.h"

#include <cstddef>

typedef enum
{
    CHARGE_FAULT = 0,
    CHARGE_INVALID1 = 1,
    CHARGE_CHARGING = 2,
    CHARGE_LOW_BATTERY_OUTPUT = 3,
    CHARGE_CHARGED = 4,
    CHARGE_INVALID5 = 5,
    CHARGE_NO_BATTERY = 6,
    CHARGE_NO_INPUT = 7,
}CHARGE_STATE;

class SystemClass
{
public:
    SystemClass(uint32_t led, uint32_t reset_user);
    void begin();
    void end();

    void setLED(bool on);
    void toggleLED();
    void pulseLED(uint32_t on_ms, uint32_t off_ms);
    inline void onLED() {setLED(true);}
    inline void offLED() {setLED(false);}

    void userInReset(bool reset);
    void ubloxReset(bool in_reset);
    void ubloxReset();

    void snooze(uint32_t ms);
    void sleep();
    void deepSleep() {deepSleep(false);}
    void deepestSleepMS(uint32_t ms);
    void deepestSleepSec(uint32_t sec);
    void deepestSleepMin(uint32_t min);
    void deepestSleepHour(uint32_t hour);
    void deepestSleepDay(uint32_t day);
    void shutdown() {deepSleep(true);}

    void timerInterrupt();
    void wakeFromSleep();
    void wakeFromSnooze();
    void chargeInterrupt(bool force=false);

    int bootVersionNumber();
    String bootVersion();

    CHARGE_STATE chargeState();
    void attachChargeNotify(void (*onChargeChange)(CHARGE_STATE));

protected:
    uint32_t led;
    uint32_t reset_user;
    uint32_t on_clocks;
    uint32_t off_clocks;
    bool pulse_on;
    bool ready;
    volatile bool sleeping;
    volatile CHARGE_STATE charge_state;
    void (*charge_state_notify)(CHARGE_STATE);

    void writeLED(bool on){digitalWrite(led, on ? HIGH : LOW);}
    void deepSleep(bool halt);
    void pulseInterrupt();
    void do_snooze(uint32_t ticks);
};
