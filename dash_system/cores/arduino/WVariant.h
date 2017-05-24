/*
  WVariant.h

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

#include <stdint.h>
#include "hal/fsl_device_registers.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SWDCLK                           0
#define UBLOX_RESET                      1
#define CHG_ST1                          2
#define SWDIO                            3
#define CHG_ST2                          4
#define M1_TMS_SWDIO                     5
#define PWM_R                            6
#define M1_TX_TO_M2_RX                   7
#define M2_TX_TO_M1_RX                   8
#define USER_RESET                       9
#define WAKE_SYSTEM                     10
#define WAKE_USER                       11
#define PWM_G                           12
#define PWM_B                           13
#define M2_UBLOX_RX                     14
#define M2_UBLOX_TX                     15
#define BL_SCL                          16
#define BL_SDA                          17
#define M2_UBLOX_CTS                    18
#define M2_UBLOX_RTS                    19
#define EZP_CS                          20
#define M1_TCK_SWDCLK                   21
#define M1_TDI                          22
#define M1_TDO_SWO                      23
#define CHG_PG                          24

#define WAKE_M1                         WAKE_USER
#define WAKE_M2                         WAKE_SYSTEM

#define A0                               5
#define A1                               6
#define A2                              13
#define A3                              14
#define A4                              16
#define A5                              17
#define A6                              20
#define A7                              21
#define A8                              22
#define A9                              23
#define A10                             24

#define PINS_COUNT                      (25u)
#define INTERRUPT_PIN_COUNT             (16u)

extern bool DASH_1_2;

void wvariant_init(void);

typedef struct
{
    uint32_t id;
    uint32_t device;
    uint8_t  major;
    uint8_t  minor;
    uint8_t  revision;
    uint8_t  pad;
    uint32_t valid;
    char     manuf[16];
    char     product[16];
    char     role[16];
    char     proc[16];
    char     desc[32];
}hologram_flash_id_t;

extern const hologram_flash_id_t FLASH_ID;

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include "Lpuart.h"
#include "Spi.h"
#include "MCUFlash.h"
#include "EZPort.h"
#include "Wire.h"
#include "System.h"
#include "Clock.h"
#include "Tricolor.h"

extern Lpuart Serial;
extern Lpuart SerialUBlox;
extern TwoWire Wire;
extern MCUFlash MCUFLASH;
extern Spi SPI;
extern EZPort EZPORT;
extern SystemClass System;
extern ClockClass Clock;
extern Tricolor RGB;
#endif
