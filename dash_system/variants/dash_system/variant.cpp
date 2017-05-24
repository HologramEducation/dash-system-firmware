/*
  variant.cpp - Set static flash identification for Dash System

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
#include "variant.h"

#ifdef __cplusplus
extern "C" {
#endif

    __attribute__((used)) const hologram_flash_id_t __attribute__((section (".idSection"))) FLASH_ID  __attribute__ ((aligned (4))) = {
        0x30505041, //APP0
        0x324D5044, //DPM2
        0,
        0,
        0,
        0,
        0,
        "HOLOGRAM.IO",
        "DASH",
        "CUSTOM",
        "SYSTEM",
        "SYSTEM CUSTOM"
    };

#ifdef __cplusplus
}
#endif

void initVariant(void)
{
}
