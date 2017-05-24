/*
  system_MKL17z4.c - Initialize the system clocks and provide
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

#include <stdint.h>
#include "hal/fsl_device_registers.h"

static const Clock_Config CLOCK_CONFIG[2] =
{
    //48MHz RUN mode
    {
        48000000u,          //System_Clock
        24000000u,          //Bus_Clock
        4000000u,           //MCGIRCLK_Clock
        MCG_MODE_HIRC,      //IR High Clock Mode
        0x03U,              //MCG_C1: CLKS=0,IRCLKEN=1,IREFSTEN=1
        0x01U,              //MCG_C2: RANGE0=0,HGO0=0,EREFS0=0,IRCS=1
        0x02U,              //MCG_SC: FCRDIV=1
        0x80U,              //MCG_MC: HIRCEN=1,LIRC_DIV2=0
        0x02U,              //SMC_PMCTRL: RUNM=0,STOPA=0,STOPM=2
        0x00010000U         //SIM_CLKDIV1: OUTDIV1=0,OUTDIV4=1
    },
    //4MHz VLPR mode
    {
        4000000u,           //System_Clock
        1000000u,           //Bus_Clock
        4000000u,           //MCGIRCLK_Clock
        MCG_MODE_LIRC_8M,   //IR Low Clock Mode
        0x43U,              //MCG_C1: CLKS=1,IRCLKEN=1,IREFSTEN=1
        0x01U,              //MCG_C2: RANGE0=0,HGO0=0,EREFS0=0,IRCS=1
        0x02U,              //MCG_SC: FCRDIV=1
        0x00U,              //MCG_MC: HIRCEN=0,LIRC_DIV2=0
        0x42U,              //SMC_PMCTRL: RUNM=2,STOPA=0,STOPM=2
        0x00030000U         //SIM_CLKDIV1: OUTDIV1=0,OUTDIV4=3
    }
};

Clock_Config const *CURRENT_CLOCK_CONFIG = &CLOCK_CONFIG[CLOCK_SETUP];

uint32_t SystemClockLookup(uint32_t index)
{
    switch(index)
    {
        case INDEX_SYSTEM_CLOCK: return CURRENT_CLOCK_CONFIG->System_Clock;
        case INDEX_BUS_CLOCK: return CURRENT_CLOCK_CONFIG->Bus_Clock;
        case INDEX_MCGIRCLK_CLOCK: return CURRENT_CLOCK_CONFIG->MCGIRCLK_Clock;
        default: return 0;
    }
}

void SystemClockSwitch(uint32_t clock_mode)
{
    if(clock_mode > CLOCK_MODE_VLPR) clock_mode = CLOCK_MODE_RUN;

    CURRENT_CLOCK_CONFIG = &CLOCK_CONFIG[clock_mode];

    if(clock_mode == CLOCK_MODE_RUN)
    {
        MCG->MC = CURRENT_CLOCK_CONFIG->MCG_MC_Value;              /* Set MC (high-frequency IRC enable, second LIRC divider) */
        SMC->PMCTRL = CURRENT_CLOCK_CONFIG->SMC_PMCTRL_Value;
        SIM->CLKDIV1 = CURRENT_CLOCK_CONFIG->SYSTEM_SIM_CLKDIV1_Value;    /* Set system prescalers */
        MCG->SC = CURRENT_CLOCK_CONFIG->MCG_SC_Value;              /* Set SC (internal reference clock divider) */
        //MCG->MC = CURRENT_CLOCK_CONFIG->MCG_MC_Value;              /* Set MC (high-frequency IRC enable, second LIRC divider) */
        MCG->C1 = CURRENT_CLOCK_CONFIG->MCG_C1_Value;              /* Set C1 (clock source selection, int. reference enable etc.) */
        MCG->C2 = CURRENT_CLOCK_CONFIG->MCG_C2_Value;              /* Set C2 (ext. and int. reference clock selection) */
        while((MCG->S & MCG_S_CLKST_MASK) != 0x00U);
        while(SMC->PMSTAT != 0x01U);      /* Wait until the system is in RUN mode */
    }
    else //VLPR
    {
        SIM->CLKDIV1 = CURRENT_CLOCK_CONFIG->SYSTEM_SIM_CLKDIV1_Value;    /* Set system prescalers */
        MCG->SC = CURRENT_CLOCK_CONFIG->MCG_SC_Value;              /* Set SC (internal reference clock divider) */
        MCG->MC = CURRENT_CLOCK_CONFIG->MCG_MC_Value;              /* Set MC (high-frequency IRC enable, second LIRC divider) */
        MCG->C1 = CURRENT_CLOCK_CONFIG->MCG_C1_Value;              /* Set C1 (clock source selection, int. reference enable etc.) */
        MCG->C2 = CURRENT_CLOCK_CONFIG->MCG_C2_Value;              /* Set C2 (ext. and int. reference clock selection) */
        while((MCG->S & MCG_S_CLKST_MASK) != 0x04U); /* Wait until low internal reference clock is selected as MCG_Lite output */
        SMC->PMCTRL = CURRENT_CLOCK_CONFIG->SMC_PMCTRL_Value;
        while(SMC->PMSTAT != 0x04U);      /* Wait until the system is in VLPR mode */
    }

    SysTick_Config(CURRENT_CLOCK_CONFIG->System_Clock/1000U);
}

void SystemInit (void) {
#if (DISABLE_WDOG)
    /* SIM->COPC: ?=0,COPCLKSEL=0,COPDBGEN=0,COPSTPEN=0,COPT=0,COPCLKS=0,COPW=0 */
    SIM->COPC = (uint32_t)0x00u;
#endif /* (DISABLE_WDOG) */

#if (ACK_ISOLATION)
    if(PMC->REGSC &  PMC_REGSC_ACKISO_MASK) {
        PMC->REGSC |= PMC_REGSC_ACKISO_MASK; /* VLLSx recovery */
    }
#endif

    /* System clock initialization */

    /* Set system prescalers and clock sources */
    SIM->SOPT1 = ((SIM->SOPT1) & (uint32_t)(~(SIM_SOPT1_OSC32KSEL_MASK))) | ((SYSTEM_SIM_SOPT1_VALUE) & (SIM_SOPT1_OSC32KSEL_MASK)); /* Set 32 kHz clock source (ERCLK32K) */
    SIM->SOPT2 = ((SIM->SOPT2) & (uint32_t)(~(
                 SIM_SOPT2_TPMSRC_MASK |
                 SIM_SOPT2_LPUART0SRC_MASK |
                 SIM_SOPT2_LPUART1SRC_MASK
                 ))) | ((SYSTEM_SIM_SOPT2_VALUE) & (
                 SIM_SOPT2_TPMSRC_MASK |
                 SIM_SOPT2_LPUART0SRC_MASK |
                 SIM_SOPT2_LPUART1SRC_MASK
                 ));   /* Select TPM, LPUARTs clock sources. */

#ifdef EXTERNAL_CLOCK_SETUP
    SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK;
    /* PORTA_PCR3: ISF=0,MUX=0 */
    PORTA_PCR18 &= (uint32_t)~(uint32_t)((PORT_PCR_ISF_MASK | PORT_PCR_MUX(0x07)));
    if (((MCG_C2_VALUE) & MCG_C2_EREFS0_MASK) != 0x00U) {
    PORTA_PCR19 &= (uint32_t)~(uint32_t)((PORT_PCR_ISF_MASK | PORT_PCR_MUX(0x07)));
#endif
    OSC0->CR = OSC0_CR_VALUE;            /* Set OSC0_CR (OSCERCLK enable, oscillator capacitor load) */

    SystemClockSwitch(CLOCK_SETUP);
}
