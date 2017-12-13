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
 *   4     |  PTA4  | --- | NMI!!!
 *   5     |  PTC1  |  A0 | USR_TMS_SWDIO
 *   6     |  PTC2  |  A1 | PWM_R (1.2 only)
 *   7     |  PTC3  | --- | USR_TX_TO_SYS_RX
 *   8     |  PTC4  | --- | SYS_TX_TO_USR_RX
 *   9     |  PTC5  | --- | USR_RESET
 *  10     |  PTC6  | --- | USER_PROGAM
 *  11     |  PTC7  | --- | WAKE_USR
 *  12     |  PTD4  | --- | PWM_G (1.2 only)
 *  13     |  PTD5  |  A2 | PWM_B (1.2 only)
 *  14     |  PTD6  |  A3 | SYS_UBLOX_RX
 *  15     |  PTD7  | --- | SYS_UBLOX_TX
 *  16     |  PTB0  |  A4 | BL_SCL
 *  17     |  PTB1  |  A5 | BL_SDA
 *  18     |  PTE0  | --- | TXL_ON_N (1.2 only)
 *  19     |  PTE1  | --- | CHG_ST2 (1.2 only)
 *  20     |  PTE16 |  A6 | EZP_CS
 *  21     |  PTE17 |  A7 | USR_TCK_SWDCLK
 *  22     |  PTE18 |  A8 | USR_TDI
 *  23     |  PTE19 |  A9 | USR_TDO_SWO
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

Lpuart SerialUBlox(LPUART0, kSimClockGateLpuart0, INDEX_MCGIRCLK_CLOCK, LPUART0_IRQn, SYS_UBLOX_RX, SYS_UBLOX_TX);
Lpuart Serial(LPUART1, kSimClockGateLpuart1, INDEX_MCGIRCLK_CLOCK, LPUART1_IRQn, USR_TX_TO_SYS_RX, SYS_TX_TO_USR_RX);

TwoWire Wire(I2C0, kSimClockGateI2c0, INDEX_SYSTEM_CLOCK, I2C0_IRQn, BL_SDA, BL_SCL);

static Spi SPI_EZPORT(SPI0, kSimClockGateSpi0, INDEX_BUS_CLOCK, SPI0_IRQn, 0, USR_TDO_SWO, USR_TDI, USR_TCK_SWDCLK);

Tricolor RGB(PWM_R, PWM_G, PWM_B, true);

MCUFlash MCUFLASH;
EZPort EZPORT(4096, 16, EZP_CS, USR_RESET);

SystemClass System(WAKE_USR, USR_RESET);
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
    System.wakeFromSleep();
}

void RTC_IRQHandler(void)
{
    Clock.alarmInterrupt();
    System.wakeFromSleep();
}

void wvariant_init(void)
{
    RCM_RPFC_REG(RCM) = 0x06;
    RCM_RPFW_REG(RCM) = 0x1F;
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

    pinMode(USER_PROGRAM, INPUT_PULLUP);
    attachInterrupt(USER_PROGRAM, NVIC_SystemReset, FALLING);

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
        pinMode(TXL_ON_N, OUTPUT);
        digitalWrite(TXL_ON_N, HIGH);
        RGB.enable(true);
    }
    Clock.init(DASH_1_2);
    if(DASH_1_2) {
        Clock.enableSeconds(true);
    }
}

#ifdef __cplusplus
}
#endif
