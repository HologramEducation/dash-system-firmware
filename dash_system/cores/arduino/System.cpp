/*
  System.cpp - Class definitions that provide special
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

#include "System.h"
#include "hal/fsl_pit_hal.h"

#include "Arduino.h"

#include <climits>

SystemClass::SystemClass(uint32_t led, uint32_t reset_user)
: led(led), reset_user(reset_user),
  on_clocks(0), off_clocks(0), pulse_on(0), ready(false), sleeping(false)
{}

void SystemClass::begin()
{
    pinMode(led, OUTPUT);
    pinMode(reset_user, INPUT_PULLUP);
    SIM_HAL_EnableClock(SIM, kSimClockGatePit0);
    PIT_HAL_Enable(PIT);
    PIT_HAL_SetTimerRunInDebugCmd(PIT, false);
    NVIC_EnableIRQ(PIT_IRQn);
    ready = true;
}

void SystemClass::end()
{
    ready = false;
    NVIC_DisableIRQ(PIT_IRQn);
    PIT_HAL_StopTimer(PIT, 0);
    setLED(false);
    pinMode(reset_user, INPUT);
}

void SystemClass::setLED(bool on)
{
    PIT_HAL_StopTimer(PIT, 0);
    writeLED(on);
}

void SystemClass::toggleLED()
{
    PIT_HAL_StopTimer(PIT, 0);
    digitalToggle(led);
}

void SystemClass::pulseLED(uint32_t on_ms, uint32_t off_ms)
{
    PIT_HAL_StopTimer(PIT, 0);
    if(on_ms == 0)
    {
        writeLED(0);
    }
    else if(off_ms == 0)
    {
        writeLED(1);
    }
    else
    {
        on_clocks = SystemBusClock/1000 * on_ms;
        off_clocks = SystemBusClock/1000 * off_ms;

        PIT_HAL_SetTimerPeriodByCount(PIT, 0, on_clocks);
        pulse_on = true;
        writeLED(1);
        PIT_HAL_StartTimer(PIT, 0);
        PIT_HAL_SetIntCmd(PIT, 0, true);
    }
}

void SystemClass::writeLED(bool on) {
    digitalWrite(led, on ? HIGH : LOW);
}

void SystemClass::userInReset(bool reset)
{
    if(reset)
    {
        pinMode(reset_user, OUTPUT);
        digitalWrite(reset_user, LOW);
    }
    else
        pinMode(reset_user, INPUT);
}

void SystemClass::ubloxReset(bool in_reset)
{
    if(in_reset)
    {
        digitalWrite(UBLOX_RESET, LOW);
        pinMode(UBLOX_RESET, OUTPUT);
    }
    else
    {
        pinMode(UBLOX_RESET, INPUT);
    }
}

void SystemClass::ubloxReset()
{
    pinMode(UBLOX_RESET, DISABLE);
    delayMicroseconds(100);
    digitalWrite(UBLOX_RESET, LOW);
    pinMode(UBLOX_RESET, OUTPUT);
    delayMicroseconds(60);
    pinMode(UBLOX_RESET, DISABLE);
}

void SystemClass::timerInterrupt()
{
    if(PIT_HAL_IsIntPending(PIT, 0))
    {
        PIT_HAL_ClearIntFlag(PIT, 0);
        pulseInterrupt();
    }
    if(PIT_HAL_IsIntPending(PIT, 1))
    {
        PIT_HAL_ClearIntFlag(PIT, 1);
        wakeFromSnooze();
    }
}

void SystemClass::pulseInterrupt()
{
    PIT_HAL_StopTimer(PIT, 0);
    PIT_HAL_ClearIntFlag(PIT, 0);
    PIT_HAL_SetTimerPeriodByCount(PIT, 0, pulse_on ? off_clocks : on_clocks);
    pulse_on = ! pulse_on;
    writeLED(pulse_on);
    PIT_HAL_SetIntCmd(PIT, 0, true);
    PIT_HAL_StartTimer(PIT, 0);
}

void SystemClass::do_snooze(uint32_t ticks)
{
    PIT_HAL_SetTimerPeriodByCount(PIT, 1, ticks);
    PIT_HAL_StartTimer(PIT, 1);
    PIT_HAL_SetIntCmd(PIT, 1, true);

    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk; //Wait mode on WFI
    while(PIT_HAL_IsTimerRunning(PIT, 1))
    {
        __DSB();
        __WFI();
        __ISB();
    }
}

void SystemClass::snooze(uint32_t ms)
{
    uint32_t max_ms = ULONG_MAX / (SystemBusClock/1000);
    uint32_t max_cycles = SystemBusClock/1000 * max_ms;

    while((ms > max_ms))
    {
        do_snooze(max_cycles);
        ms -= max_ms;
    }
    do_snooze(SystemBusClock/1000 * ms);
}

void SystemClass::wakeFromSnooze()
{
    PIT_HAL_StopTimer(PIT, 1);
    PIT_HAL_ClearIntFlag(PIT, 1);
}

void SystemClass::sleep()
{
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk; //Wait mode on WFI
    sleeping = true;
    while(sleeping)
    {
        __DSB();
        __WFI();
        __ISB();
    }
}

void SystemClass::wakeFromSleep()
{
    sleeping = false;
}

void SystemClass::deepSleep(bool halt)
{
    PIT_HAL_Disable(PIT);
    bool led_on = ((GPIO_RD_PDOR(PERIPH_GPIO(led)) >> PINS_PIN(led)) & 1U);
    writeLED(false);
    //use LPTMR, set prescaler, use 1kHz LPO (b01)
    bool enable_ir = MCG_BRD_C1_IREFSTEN(MCG); //turn off IR Clock in Stop
    MCG_BWR_C1_IREFSTEN(MCG, 0);
    NVIC_EnableIRQ(LLWU_IRQn);
    LLWU_WR_PE3_WUPE10(LLWU, 2); //Falling edge on program button
    LLWU_WR_PE2_WUPE7(LLWU, 1); //Rising edge on UART RX
    SMC_BWR_PMCTRL_STOPM(SMC_BASE_PTR, 3); //Enter LLS mode on WFI
    if(halt) {
        Clock.enableSeconds(false);
        if(DASH_1_2) {
            RGB.enable(false);
            pinMode(CHG_ST1, DISABLE);
            pinMode(CHG_ST2, DISABLE);
            pinMode(CHG_PG, DISABLE);
            pinMode(UBLOX_RESET, DISABLE);
        }
        pinMode(reset_user, DISABLE);
        pinMode(led, DISABLE);
        pinMode(SWDIO, DISABLE);
        pinMode(SWDCLK, DISABLE);
    }
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    (void)SMC->PMCTRL;
    //__DSB();
    __WFI();
    //__ISB();
    if(halt) {
        //ubloxReset();
        //NVIC_SystemReset();
    }
    NVIC_DisableIRQ(LLWU_IRQn);
    MCG_BWR_C1_IREFSTEN(MCG, enable_ir); //re-enable IR Clock in Stop
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    writeLED(led_on);
    PIT_HAL_Enable(PIT);
}

void SystemClass::deepestSleepMS(uint32_t ms)
{
    //default: no presaler
    uint32_t psr = 0x05;
    uint32_t cmr = ms;
    if(ms > 65535)
    {
        //determine prescaler
        uint32_t prescale = 0;
        for(prescale=0; prescale<15; prescale++)
        {
            uint32_t overflow = 1 << (17+prescale);
            if(ms < overflow)
                break;
        }
        psr = (prescale << LPTMR_PSR_PRESCALE_SHIFT) | 0x01;
        uint32_t ms_per_tick = 1 << (prescale+1);
        cmr = ms/ms_per_tick;
    }

    volatile uint32_t interrupt_state = NVIC->ISER[0]; //save interrupt state
    NVIC->ICER[0] = 0xFFFFFFFF; //disable all user interrupts

    SIM_HAL_EnableClock(SIM, kSimClockGateLptmr0);
    LPTMR_WR_CSR(LPTMR0, 0x00); //Reset LPTMR
    LPTMR_WR_PSR(LPTMR0, psr); //Prescaler, LPO Clock Source
    LPTMR_WR_CMR(LPTMR0, cmr); //Compare value
    LPTMR_WR_CSR(LPTMR0, 0x41); //Clear Interrupt Flag, Enable Interrupt, Enable Timer
    NVIC_EnableIRQ(LPTMR0_IRQn);

    LLWU_WR_ME(LLWU_BASE_PTR, 1);    //LPTMR

    deepSleep();

    LPTMR_WR_CSR(LPTMR0, 0x00); //Clear Interrupt Flag, Disable Interrupt, Disable Timer
    SIM_HAL_DisableClock(SIM, kSimClockGateLptmr0);
    NVIC->ISER[0] = interrupt_state; //re-enable interrupts to previous state
}

void SystemClass::deepestSleepSec(uint32_t sec)
{
    if(sec > 4294967) return;
    deepestSleepMS(sec*1000);
}

void SystemClass::deepestSleepMin(uint32_t min)
{
    if(min > 71582) return;
    deepestSleepMS(min*60000);
}

void SystemClass::deepestSleepHour(uint32_t hour)
{
    if(hour > 1193) return;
    deepestSleepMS(hour*3600000);
}

void SystemClass::deepestSleepDay(uint32_t day)
{
    if(day > 49) return;
    deepestSleepMS(day*86400000);
}

int SystemClass::bootVersionNumber()
{
    uint8_t * pversion = (uint8_t*)0x208;
    return
    ((*pversion++) << 16) |
    ((*pversion++) <<  8) |
    ((*pversion++));
}

String SystemClass::bootVersion()
{
    String major    = String(*(unsigned char*)0x208);
    String minor    = String(*(unsigned char*)0x209);
    String revision = String(*(unsigned char*)0x20A);
    return major + '.' + minor + '.' + revision;
}

CHARGE_STATE SystemClass::chargeState()
{
    if(!DASH_1_2) return CHARGE_UNKNOWN;

    uint32_t st = digitalRead(CHG_ST1) << 2;
    st |= digitalRead(CHG_ST2) << 1;
    st |= digitalRead(CHG_PG);

    return (CHARGE_STATE)st;
}

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

static volatile uint32_t gLastWake = 0;

void LLWU_IRQHandler(void)
{
    if(LLWU_RD_F2_WUF10(LLWU)) {NVIC_SystemReset();}

    gLastWake = (LLWU_F3 << 16) | (LLWU_F2 << 8) | LLWU_F1;

    LLWU_F1 = 0xFF;
    LLWU_F2 = 0xFF;

    if(LLWU_F3 & LLWU_F3_MWUF0_MASK)
    {
        if(LPTMR0_CSR & LPTMR_CSR_TCF_MASK)
        {
            LPTMR_SET_CSR(LPTMR0, LPTMR_CSR_TCF_MASK);
        }
    }

    //SystemInit();
}

uint32_t SystemClass::getLastWake() {
    return gLastWake;
}

#ifdef __cplusplus
}
#endif
