/*
  Updater.cpp - Class definitions that provide a link to the bootloader for
  System and User MCU flash program updates

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
#include "Updater.h"
#include <string.h>
#include "MCUFlash.h"
#include "WVariant.h"

#define BOOT_SPECIAL_CODE 0x544F4F42  //BOOT
#define APP_ADDRESS       0x00006000
#define BOOT_FLAG_ADDRESS (APP_ADDRESS - FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE)

void Updater::init(EZPort &ezport)
{
    this->ezport = &ezport;
    memset(&boot_flags, 0xFF, sizeof(boot_flags));
}

bool Updater::updateUserModule(uint32_t dst, uint32_t src, uint32_t count)
{
    if(!ezport->ready())
        ezport->begin();
    return ezport->copyFrom(dst, src, count);
}

bool Updater::updateUserModule(uint32_t dst, Stream &stream, uint32_t count)
{
    if(!ezport->ready())
        ezport->begin();
    return ezport->copyFrom(stream, dst, count);
}

void Updater::updateSystemBoot(Stream &stream, uint32_t count)
{
    if(count > BOOT_FLAG_ADDRESS) count = BOOT_FLAG_ADDRESS;

    uint32_t dst = 0;
    uint32_t sectormask = (FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE-1);
    uint8_t buffer[FSL_FEATURE_FLASH_PFLASH_BLOCK_WRITE_UNIT_SIZE];
    while(dst < count)
    {
        if((dst & sectormask) == 0)
            MCUFLASH.eraseSector(dst);

        memset(buffer, 0xFF, FSL_FEATURE_FLASH_PFLASH_BLOCK_WRITE_UNIT_SIZE);

        for(int i=0; i<FSL_FEATURE_FLASH_PFLASH_BLOCK_WRITE_UNIT_SIZE; i++)
        {
            int c = stream.read();
            if(c < 0) break;
            buffer[i] = (uint8_t)c;
        }
        MCUFLASH.write(dst, buffer, FSL_FEATURE_FLASH_PFLASH_BLOCK_WRITE_UNIT_SIZE);

        dst += FSL_FEATURE_FLASH_PFLASH_BLOCK_WRITE_UNIT_SIZE;
    }
}

void Updater::updateSystemBoot(uint32_t src, uint32_t count)
{
    if(count > BOOT_FLAG_ADDRESS) count = BOOT_FLAG_ADDRESS;

    uint32_t dst = 0;
    uint8_t *srcbuff = (uint8_t*)src;
    while(dst < count)
    {
        MCUFLASH.eraseSector(dst);
        MCUFLASH.write(dst, srcbuff, FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE);

        dst += FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE;
        srcbuff += FSL_FEATURE_FLASH_PFLASH_BLOCK_SECTOR_SIZE;
    }
}

void Updater::updateUserOnReset(const char* filename, uint32_t count, uint32_t offset)
{
    strcpy(boot_flags.user_filename, filename);
    boot_flags.user_size = count;
    boot_flags.user_offset = offset;
}

void Updater::updateUserBootOnReset(const char* filename, uint32_t count, uint32_t offset)
{
    if(count > BOOT_FLAG_ADDRESS) count = BOOT_FLAG_ADDRESS;
    strcpy(boot_flags.userboot_filename, filename);
    boot_flags.userboot_size = count;
    boot_flags.userboot_offset = offset;
}

void Updater::updateSystemOnReset(const char* filename, uint32_t count, uint32_t offset)
{
    strcpy(boot_flags.system_filename, filename);
    boot_flags.system_size = count;
    boot_flags.system_offset = offset;
}

void Updater::updateSystemInternalOnReset(uint32_t src, uint32_t count)
{
    boot_flags.internal_system_src = src;
    boot_flags.internal_system_size = count;
}

void Updater::updateReset()
{
    if(ezport->ready())
        ezport->end();
    boot_flags.special_code = BOOT_SPECIAL_CODE;
    boot_flags.end_code = BOOT_SPECIAL_CODE;
    MCUFLASH.eraseSector(BOOT_FLAG_ADDRESS);
    MCUFLASH.write(BOOT_FLAG_ADDRESS, (uint8_t*)&boot_flags, sizeof(konekt_boot_flags_t));
    NVIC_SystemReset();
}
