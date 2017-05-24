/*
  WVariant.cpp

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

#include "WVariant.h"
#include "WInterrupts.h"

/*
 * Arduino |  PORT  | ANA | Description
 * --------+--------+-----+---------------------------------------
 *   0     |  PTA0  | --- | M2_TCK_SWDCLK
 *   1     |  PTA1  | --- | UBLOX_RESET
 *   2     |  PTA2  | --- | CHG_ST1 (1.2 only)
 *   3     |  PTA3  | --- | M2_TMS_SWDIO
 *   4     |  PTA4  | --- | CHG_ST2 (1.2 only)  NMI!!!
 *   5     |  PTC1  |  A0 | M1_TMS_SWDIO
 *   6     |  PTC2  |  A1 | PWM_R (1.2 only)
 *   7     |  PTC3  | --- | M1_TX_TO_M2_RX
 *   8     |  PTC4  | --- | M2_TX_TO_M1_RX
 *   9     |  PTC5  | --- | M1_RESET
 *  10     |  PTC6  | --- | WAKE_M2
 *  11     |  PTC7  | --- | WAKE_M1
 *  12     |  PTD4  | --- | PWM_G (1.2 only)
 *  13     |  PTD5  |  A2 | PWM_B (1.2 only)
 *  14     |  PTD6  |  A3 | M2_UBLOX_RX
 *  15     |  PTD7  | --- | M2_UBLOX_TX
 *  16     |  PTB0  |  A4 | BL_SCL
 *  17     |  PTB1  |  A5 | BL_SDA
 *  18     |  PTE0  | --- | M2_UBLOX_CTS (1.2 only)
 *  19     |  PTE1  | --- | M2_UBLOX_RTS (1.2 only)
 *  20     |  PTE16 |  A6 | EZP_CS
 *  21     |  PTE17 |  A7 | M1_TCK_SWDCLK
 *  22     |  PTE18 |  A8 | M1_TDI
 *  23     |  PTE19 |  A9 | M1_TDO_SWO
 *  24     |  PTE30 | A10 | CHG_PG
 */

extern const PinDescription g_APinDescription[]=
{
//     PORT   PIN  WAKE                     ANALOG  UART   I2C   SPI        PWM
    {PORT_A,    0, NONE,                      NONE, NONE, NONE, NONE, PWM_MUX(PWM_0, 5, MUX3)}, // 0
    {PORT_A,    1, NONE,                      NONE, NONE, NONE, NONE, PWM_MUX(PWM_2, 0, MUX3)}, // 1
    {PORT_A,    2, NONE,                      NONE, NONE, NONE, NONE, PWM_MUX(PWM_2, 1, MUX3)}, // 2
    {PORT_A,    3, NONE,                      NONE, NONE, NONE, NONE, PWM_MUX(PWM_0, 0, MUX3)}, // 3
    {PORT_A,    4, NONE,                      NONE, NONE, NONE, NONE, PWM_MUX(PWM_0, 1, MUX3)}, // 4
    {PORT_C,    1,    6, ADC_PIN(ADC_0, 15),        NONE, NONE, NONE, PWM_MUX(PWM_0, 0, MUX4)}, // 5
    {PORT_C,    2, NONE, ADC_PIN(ADC_0, 11),        NONE, NONE, NONE, PWM_MUX(PWM_0, 1, MUX4)}, // 6
    {PORT_C,    3,    7,                      NONE, MUX3, NONE, NONE, PWM_MUX(PWM_0, 2, MUX4)}, // 7
    {PORT_C,    4,    8,                      NONE, MUX3, NONE, NONE, PWM_MUX(PWM_0, 3, MUX4)}, // 8
    {PORT_C,    5,    9,                      NONE, NONE, NONE, NONE,                    NONE}, // 9
    {PORT_C,    6,   10,                      NONE, NONE, NONE, NONE,                    NONE}, //10
    {PORT_C,    7, NONE,                      NONE, NONE, NONE, NONE,                    NONE}, //11
    {PORT_D,    4,   14,                      NONE, NONE, NONE, NONE, PWM_MUX(PWM_0, 4, MUX4)}, //12
    {PORT_D,    5, NONE, ADC_MUX(ADC_0,  6, ADC_B), NONE, NONE, NONE, PWM_MUX(PWM_0, 5, MUX4)}, //13
    {PORT_D,    6,   15, ADC_MUX(ADC_0,  7, ADC_B), MUX3, NONE, NONE,                    NONE}, //14
    {PORT_D,    7, NONE,                      NONE, MUX3, NONE, NONE,                    NONE}, //15
    {PORT_B,    0,    5, ADC_PIN(ADC_0,  8),        NONE, MUX2, NONE, PWM_MUX(PWM_1, 0, MUX3)}, //16
    {PORT_B,    1, NONE, ADC_PIN(ADC_0,  9),        NONE, MUX2, NONE, PWM_MUX(PWM_1, 1, MUX3)}, //17
    {PORT_E,    0, NONE,                      NONE, NONE, NONE, NONE,                    NONE}, //18
    {PORT_E,    1, NONE,                      NONE, NONE, NONE, NONE,                    NONE}, //19
    {PORT_E,   16, NONE, ADC_PIN(ADC_0,  1),        NONE, NONE, NONE,                    NONE}, //21
    {PORT_E,   17, NONE, ADC_MUX(ADC_0,  5, ADC_A), NONE, NONE, MUX2,                    NONE}, //21
    {PORT_E,   18, NONE, ADC_PIN(ADC_0,  2),        NONE, NONE, MUX2,                    NONE}, //22
    {PORT_E,   19, NONE, ADC_MUX(ADC_0,  6, ADC_A), NONE, NONE, MUX2,                    NONE}, //23
    {PORT_E,   30, NONE, ADC_PIN(ADC_0, 23),        NONE, NONE, NONE,              PWM_DAC(0)}, //24
};

Lpuart SerialUBlox(LPUART0, kSimClockGateLpuart0, INDEX_MCGIRCLK_CLOCK, LPUART0_IRQn, M2_UBLOX_RX, M2_UBLOX_TX);
Lpuart Serial(LPUART1, kSimClockGateLpuart1, INDEX_MCGIRCLK_CLOCK, LPUART1_IRQn, M1_TX_TO_M2_RX, M2_TX_TO_M1_RX);

TwoWire Wire(I2C0, kSimClockGateI2c0, INDEX_SYSTEM_CLOCK, I2C0_IRQn, BL_SDA, BL_SCL);

static Spi SPI_EZPORT(SPI0, kSimClockGateSpi0, INDEX_BUS_CLOCK, SPI0_IRQn, 0, M1_TDO_SWO, M1_TDI, M1_TCK_SWDCLK);

Tricolor RGB(PWM_R, PWM_G, PWM_B, true);

MCUFlash MCUFLASH;
EZPort EZPORT(4096, 16, EZP_CS, USER_RESET);

SystemClass System(WAKE_USER, USER_RESET);
ClockClass Clock;

bool DASH_1_2 = false;

#ifdef __cplusplus
extern "C" {
#endif

void __cxa_pure_virtual() { while (1); }

void LPUART0_IRQHandler(void)
{
    SerialUBlox.IrqHandler();
    System.wakeFromSleep();
}

void LPUART1_IRQHandler(void)
{
    Serial.IrqHandler();
    System.wakeFromSleep();
}

void I2C0_IRQHandler(void)
{
    Wire.onService();
    System.wakeFromSleep();
}

void NMI_Handler(void)
{
    System.chargeInterrupt();
}

void PIT_IRQHandler(void)
{
    handleInterrupt(PORT_A);
    System.timerInterrupt();
}

void PORTA_IRQHandler(void)
{
    handleInterrupt(PORT_A);
}

void PORTCD_IRQHandler(void)
{
    handleInterrupt(PORT_C);
    handleInterrupt(PORT_D);
}

void LPTMR0_IRQHandler(void)
{
    LPTMR_WR_CSR(LPTMR0, 0xC1); //Clear Interrupt Flag, Disable Interrupt, Disable Timer
}

void RTC_Seconds_IRQHandler(void)
{
    Clock.secondsInterrupt();
}

void RTC_IRQHandler(void)
{
    Clock.alarmInterrupt();
    System.wakeFromSleep();
}

void charge_state_changed(void)
{
    System.chargeInterrupt();
}

void wvariant_init(void)
{
    NVIC_SetPriority(LPTMR0_IRQn, 0);
    NVIC_SetPriority(LPUART0_IRQn, 1);
    NVIC_SetPriority(LPUART1_IRQn, 1);
    NVIC_SetPriority(I2C0_IRQn, 3);
    NVIC_SetPriority(SPI0_IRQn, 2);
    NVIC_SetPriority(SPI1_IRQn, 2);
    NVIC_SetPriority(PIT_IRQn, 2);
    NVIC_SetPriority(PORTA_IRQn, 2);
    NVIC_SetPriority(PORTCD_IRQn, 2);

    CLOCK_HAL_SetLpuartSrc(SIM, 0, kClockLpuartSrcMcgIrClk);
    CLOCK_HAL_SetLpuartSrc(SIM, 1, kClockLpuartSrcMcgIrClk);

    EZPORT.init(SPI_EZPORT);
    System.begin();

    pinMode(WAKE_SYSTEM, INPUT_PULLUP);
    attachInterrupt(WAKE_SYSTEM, NVIC_SystemReset, FALLING);

    pinMode(CHG_ST1, INPUT_PULLDOWN);
    pinMode(CHG_ST2, INPUT_PULLDOWN);
    pinMode(CHG_PG, INPUT_PULLDOWN);

    bool all_lo = (digitalRead(CHG_ST1) == LOW &&
                   digitalRead(CHG_ST2) == LOW &&
                   digitalRead(CHG_PG) == LOW);

    pinMode(CHG_ST1, INPUT_PULLUP);
    pinMode(CHG_ST2, INPUT_PULLUP);
    pinMode(CHG_PG, INPUT_PULLUP);

    bool all_hi = (digitalRead(CHG_ST1) == HIGH &&
                   digitalRead(CHG_ST2) == HIGH &&
                   digitalRead(CHG_PG) == HIGH);

    pinMode(CHG_ST1, INPUT);
    pinMode(CHG_ST2, INPUT);
    pinMode(CHG_PG, INPUT);

    DASH_1_2 = !(all_lo & all_hi);

    if(DASH_1_2)
    {
        //SerialUBlox.flowcontrol(true, M2_UBLOX_RTS, M2_UBLOX_CTS);
        RGB.enable(true);
        attachInterrupt(CHG_ST1, charge_state_changed, CHANGE);
        attachInterrupt(CHG_ST2, charge_state_changed, CHANGE);
        System.chargeInterrupt(true);
    }
    Clock.init(DASH_1_2);
}

#ifdef __cplusplus
}
#endif
