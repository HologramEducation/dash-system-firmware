/*
  MCUFlash.cpp - Class definitions that provide a Flash subclass for the internal
  flash memory for a Kinetis MCU.

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
#include "MCUFlash.h"
#include "wiring_digital.h"

static const FLASH_SSD_CONFIG flashconfig =
{
    .ftfxRegBase = 1073872896U,
    .PFlashBase  = 0U,
    .PFlashSize  = 262144U,
    .DFlashBase  = 0U,
    .DFlashSize  = 0x00U,
    .EERAMBase   = 0U,
    .EEESize     = 0x00U,
    .DebugEnable = false,
    .CallBack    = NULL_CALLBACK
};

MCUFlash::MCUFlash()
:Flash(1024, 256), readPtr(0), writeCount(0), writeAddress(0){}

void MCUFlash::begin()
{
    g_FlashLaunchCommand = (pFLASHCOMMANDSEQUENCE)RelocateFunction((uint32_t)ramFunc, 50, (uint32_t)FlashCommandSequence);
    begun = true;
}

void MCUFlash::read(uint32_t address, uint8_t *buffer, size_t count)
{
    if(!ready()) return;
    memcpy(buffer, (void*)address, count);
}

uint8_t MCUFlash::write(uint32_t address, uint8_t *buffer, size_t count)
{
    if(!ready()) return 0xFF;
    __disable_irq();
    FlashProgram(&flashconfig, address, count, buffer, g_FlashLaunchCommand);
    __enable_irq();
    return 0;
}

uint8_t MCUFlash::eraseSector(uint32_t address)
{
    if(!ready()) return 0xFF;
    __disable_irq();
    FlashEraseSector(&flashconfig, address, FTFx_PSECTOR_SIZE, g_FlashLaunchCommand);
    __enable_irq();
}

uint8_t MCUFlash::eraseAll()
{
    return 0xFF;
}

void MCUFlash::beginRead(uint32_t address)
{
    readPtr = (uint8_t*)address;
}

uint8_t MCUFlash::continueRead()
{
    return *readPtr++;
}

void MCUFlash::beginWrite(uint32_t address)
{
    if(!ready()) return;
    writeCount = 0;
    writeAddress = address;
}

void MCUFlash::continueWrite(uint8_t byte)
{
    if(writeCount == PGM_SIZE_BYTE)
    {
        endWrite();
        writeAddress += PGM_SIZE_BYTE;
    }
    writeBuffer[writeCount++] = byte;
}

void MCUFlash::endWrite()
{
    if(writeCount)
    {
        write(writeAddress, writeBuffer, writeCount);
        writeCount = 0;
    }
}
