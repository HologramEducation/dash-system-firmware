/*
  FlashStore.h - Class definitions that provide a key/value pair table in
  a Flash memory

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

#include "Flash.h"

class FlashStore
{
public:
    FlashStore(Flash &flash);
    void begin(uint32_t sector);
    void end();
    bool load();
    void create(uint32_t sectors);

    bool find(const String &key, String *value);
    bool find(const char *key, String *value) { return find(String(key), value); }

    bool add(const String &key, const String &value, bool autopurge=true);
    bool add(const String &key, const char *value, bool autopurge=true) {return add(key,String(value),autopurge);}
    bool add(const char *key, const String &value, bool autopurge=true) {return add(String(key),value,autopurge);}
    bool add(const char *key, const char *value, bool autopurge=true) {return add(String(key),String(value),autopurge);}

    bool remove(const String &key);
    bool remove(const char *key) { return remove(String(key)); }

    bool purge();
    bool erase();

    bool isValid();
    uint32_t sectorSize() {return flash->getSectorSize();}
    uint32_t available();
    uint32_t reclaimable();
    uint32_t numKeys() {return num_keys;}

private:
    typedef struct
    {
        uint32_t id;
        uint32_t count;
        uint32_t size;
    }flash_store_header_t;

    typedef struct
    {
        uint16_t keylen;
        uint16_t vallen;
        uint32_t valid;
    }entry_t;

    Flash *flash;
    flash_store_header_t header;
    uint32_t start_address;
    uint32_t first_key_address;
    bool loaded;
    uint32_t num_keys;
    uint32_t reclaim;
    uint32_t free_key_address;
    uint32_t block_address;

    bool findFrom(const String &key, uint32_t *address, String *value);
    bool findFrom(const char *key, uint32_t *address, String *value) { return findFrom(String(key), address, value); }

    uint32_t findFree();
    uint16_t storedLength(uint16_t len);
    uint32_t recordLength(entry_t *entry);

    uint32_t recordLength(const String &key, const String &val);
    uint32_t recordLength(const char *key, const char *val) { return recordLength(String(key), String(val)); }
};
