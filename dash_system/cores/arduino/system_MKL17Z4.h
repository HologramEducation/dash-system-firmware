/*
  system_MKL17z4.h - Initialize the system clocks and provide
  functions to query and change the clock mode.

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
#ifndef SYSTEM_MKL17Z4_H_
#define SYSTEM_MKL17Z4_H_                     /**< Symbol preventing repeated inclusion */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifndef DISABLE_WDOG
  #define DISABLE_WDOG                 1
#endif

#define ACK_ISOLATION                  0

/* Index of selected clock configuration */
#ifndef CLOCK_SETUP
  #define CLOCK_SETUP   CLOCK_MODE_RUN
#endif

#define CLOCK_MODE_RUN  0
#define CLOCK_MODE_VLPR 1

/* MCG_Lite mode constants */

#define MCG_MODE_LIRC_8M               0U
#define MCG_MODE_HIRC                  1U
#define MCG_MODE_LIRC_2M               2U
#define MCG_MODE_EXT                   3U

#define INDEX_SYSTEM_CLOCK      0
#define INDEX_BUS_CLOCK         1
#define INDEX_MCGIRCLK_CLOCK    2

/* Low power mode enable, already set in boot loader */
/* SMC_PMPROT: AVLP=1,ALLS=0,AVLLS=1 */
#define SYSTEM_SMC_PMPROT_VALUE        0x22U               /* SMC_PMPROT */
/* SIM_SOPT1: OSC32KSEL=3,OSC32KOUT=0 */
#define SYSTEM_SIM_SOPT1_VALUE       0x00C00000U         /* SIM_SOPT1 */
/* SIM_SOPT2: LPUART1SRC=0,LPUART0SRC=0,TPMSRC=0,FLEXIOSRC=0,USBSRC=0,CLKOUTSEL=0,RTCCLKOUTSEL=0 */
#define SYSTEM_SIM_SOPT2_VALUE       0x00U               /* SIM_SOPT2 */
/* OSC0_CR: ERCLKEN=0,EREFSTEN=0,SC2P=0,SC4P=0,SC8P=0,SC16P=0 */
#define OSC0_CR_VALUE                0x00U               /* OSC0_CR */

typedef struct
{
    uint32_t System_Clock;
    uint32_t Bus_Clock;
    uint32_t MCGIRCLK_Clock;
    uint8_t  MCG_Mode;
    uint8_t  MCG_C1_Value;
    uint8_t  MCG_C2_Value;
    uint8_t  MCG_SC_Value;
    uint8_t  MCG_MC_Value;
    uint8_t  SMC_PMCTRL_Value;
    uint32_t SYSTEM_SIM_CLKDIV1_Value;
}Clock_Config;

void SystemInit (void);

void SystemClockSwitch(uint32_t clock_mode);

uint32_t SystemClockLookup(uint32_t index);

#define SystemCoreClock (SystemClockLookup(INDEX_SYSTEM_CLOCK))
#define SystemBusClock (SystemClockLookup(INDEX_BUS_CLOCK))
#define SystemIRClock (SystemClockLookup(INDEX_MCGIRCLK_CLOCK))

#ifdef __cplusplus
}
#endif

#endif  /* #if !defined(SYSTEM_MKL17Z4_H_) */
