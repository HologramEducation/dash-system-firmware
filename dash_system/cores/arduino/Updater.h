/*
  Updater.h - Class definitions that provide a link to the bootloader for
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
#pragma once

#include "EZPort.h"
#include "Stream.h"

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus
#include "SSD_FTFx.h"
#ifdef __cplusplus
}
#endif // __cplusplus

class Updater
{
public:
    void init(EZPort &ezport);

    bool updateUserModule(uint32_t dst, Stream &stream, uint32_t count);
    bool updateUserModule(uint32_t dst, uint32_t src, uint32_t count);

    bool updateUserApplication(Stream &stream, uint32_t count) {return updateUserModule(0x8000, stream, count);}
    bool updateUserApplication(uint32_t src, uint32_t count) {return updateUserModule(0x8000, src, count);}

    bool updateUserBoot(Stream &stream, uint32_t count) {return updateUserModule(0x0, stream, count);}
    bool updateUserBoot(uint32_t src, uint32_t count) {return updateUserModule(0x0, src, count);}

    void updateSystemBoot(Stream &stream, uint32_t count);
    void updateSystemBoot(uint32_t src, uint32_t count);

    void updateUserOnReset(const char* filename, uint32_t count, uint32_t offset=0);
    void updateUserBootOnReset(const char* filename, uint32_t count, uint32_t offset=0);
    void updateSystemOnReset(const char* filename, uint32_t count, uint32_t offset=0);

    void updateSystemInternalOnReset(uint32_t src, uint32_t count);

    void updateReset();

protected:
    typedef struct
    {
        uint32_t special_code;
        uint32_t userboot_size;
        uint32_t userboot_offset;
        char     userboot_filename[256];
        uint32_t user_size;
        uint32_t user_offset;
        char     user_filename[256];
        uint32_t system_size;
        uint32_t system_offset;
        char     system_filename[256];
        uint32_t internal_system_src;
        uint32_t internal_system_size;
        uint32_t end_code;
    }konekt_boot_flags_t;

    EZPort *ezport;
    konekt_boot_flags_t boot_flags;
};
