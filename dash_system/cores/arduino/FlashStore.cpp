/*
  FlashStore.cpp - Class definitions that provide a key/value pair table in
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
#include "FlashStore.h"

#define STORE_ID_A  (0x41A0A041)
#define STORE_ID_B  (0x42B0B042)

FlashStore::FlashStore(Flash &flash)
: flash(&flash), num_keys(0), reclaim(0), loaded(0),
first_key_address(0), free_key_address(0), block_address(0),
start_address(0xFFFFFFFF) {}

void FlashStore::begin(uint32_t sector)
{
    start_address = sector*(flash->getSectorSize());
}

void FlashStore::end()
{
}

void FlashStore::create(uint32_t sectors)
{
    for(int i=0; i<sectors*2; i++)
    {
        flash->eraseSector(start_address+i*sectorSize());
    }
    header.id = STORE_ID_B;
    header.size = sectors * sectorSize();
    header.count = 0;
    flash->write(start_address+header.size, (uint8_t*)&header, sizeof(header));

    header.id = STORE_ID_A;
    header.count = 1;
    flash->write(start_address, (uint8_t*)&header, sizeof(header));

    first_key_address = start_address + sizeof(flash_store_header_t);
    free_key_address = first_key_address;
    num_keys = 0;
    block_address = start_address;
    loaded = true;
}

uint16_t FlashStore::storedLength(uint16_t len)
{
    if((len % 4) == 0) return len;
    return len+(4-len%4);
}

uint32_t FlashStore::recordLength(entry_t *entry)
{
    return sizeof(*entry) + storedLength(entry->keylen) + storedLength(entry->vallen);
}

uint32_t FlashStore::recordLength(const String &key, const String &val)
{
    return sizeof(entry_t) + storedLength(key.length()) + storedLength(val.length());
}

bool FlashStore::load()
{
    loaded = false;
    flash_store_header_t header_b;

    flash->read(start_address, (uint8_t*)&header, sizeof(flash_store_header_t));
    if(header.id != STORE_ID_A)
        return false;

    flash->read(start_address+header.size, (uint8_t*)&header_b, sizeof(flash_store_header_t));
    if(header_b.id != STORE_ID_B)
        return false;

    if(header.count == header_b.count || header.size != header_b.size)
        return false;

    first_key_address = start_address + sizeof(flash_store_header_t);
    block_address = start_address;
    if(header_b.count > header.count)
    {
        //switch to b
        memcpy(&header, &header_b, sizeof(flash_store_header_t));
        first_key_address += header.size;
        block_address += header.size;
    }

    entry_t entry;
    uint32_t address = first_key_address;
    while(address < block_address + header.size)
    {
        flash->read(address, (uint8_t*)&entry, sizeof(entry));
        if(entry.keylen == 0xFFFF)
            break;
        if(entry.valid)
            num_keys++;
        else
        {
            reclaim += storedLength(entry.keylen) + storedLength(entry.vallen) + sizeof(entry);
        }
        address += recordLength(&entry);
    }
    free_key_address = address;

    loaded = true;
    return true;
}

bool FlashStore::isValid()
{
    return loaded;
}

uint32_t FlashStore::reclaimable()
{
    return reclaim;
}

uint32_t FlashStore::available()
{
    return loaded ? block_address + header.size - free_key_address : 0;
}

bool FlashStore::findFrom(const String &key, uint32_t *address, String *value)
{
    if(!loaded) return false;

    entry_t entry;
    while(*address < block_address + header.size)
    {
        flash->read(*address, (uint8_t*)&entry, sizeof(entry));
        if(entry.keylen == 0xFFFF)
            return false; //no more entries
        if(entry.valid == 0xFFFFFFFF &&  //valid
            key.length() == entry.keylen) //same
        {
            if(flash->compareString(*address+sizeof(entry), key))
            {
                *value = flash->readString(*address+sizeof(entry)+storedLength(entry.keylen), entry.vallen);
                return true;
            }
        }
        *address += recordLength(&entry);
    }
    return false;
}

bool FlashStore::find(const String &key, String *value)
{
    uint32_t address = first_key_address;
    return findFrom(key, &address, value);
}

bool FlashStore::add(const String &key, const String &value, bool autopurge)
{
    //look for existing key
    if(!loaded) return false;
    if(key.length() > 254) return false;
    if(value.length() > 254) return false;
    if(free_key_address == 0) return false;
    uint32_t space_needed = recordLength(key, value);
    if(space_needed > available())
    {
        bool hasspace = false;
        if(autopurge && (space_needed <= reclaimable()))
            hasspace = purge();
        if(!hasspace)
            return false;
    }

    uint32_t address = first_key_address;
    entry_t entry = {(uint16_t)key.length(), (uint16_t)value.length()};

    String s;
    if(findFrom(key, &address, &s))
    {
        if(value.equals(s))
            return true;
        //mark that string as invalid
        uint32_t invalid = 0x00;
        flash->write(address+4, (uint8_t*)&invalid, 4);
        num_keys--;
        reclaim += recordLength(key, s);
    }

    flash->write(free_key_address, (uint8_t*)&entry, 4);
    flash->writeString(free_key_address+sizeof(entry), key);
    flash->writeString(free_key_address+sizeof(entry)+storedLength(key.length()), value);
    free_key_address += recordLength(key, value);
    num_keys++;
    return true;
}

bool FlashStore::remove(const String &key)
{
    uint32_t address = first_key_address;
    String s;
    if(findFrom(key, &address, &s))
    {
        entry_t entry;
        entry.valid = 0x00;
        flash->write(address+4, (uint8_t*)&entry.valid, 4);
        flash->read(address, (uint8_t*)&entry, 4);
        reclaim += recordLength(&entry);
        num_keys--;
        return true;
    }
    return false;
}

bool FlashStore::purge()
{
    if(reclaim == 0) return false;
    //figure out if in a or b
    uint32_t read_address = first_key_address;
    uint32_t write_address = start_address;
    if(header.id == STORE_ID_A)
        write_address += header.size;

    uint32_t erase_address = write_address;
    while(erase_address < write_address + header.size)
    {
        flash->eraseSector(erase_address);
        erase_address += sectorSize();
    }
    uint32_t write_header_address = write_address;
    write_address += sizeof(header);
    uint32_t keys_left = num_keys;

    while(keys_left && read_address < block_address + header.size)
    {
        entry_t entry;
        flash->read(read_address, (uint8_t*)&entry, sizeof(entry));
        read_address += sizeof(entry);
        if(entry.keylen == 0xFFFF)
            break; //ERROR: should be no keys left
        if(entry.valid) //copy it to purge
        {
            uint8_t buffer[32];
            uint32_t left = storedLength(entry.keylen) + storedLength(entry.vallen);
            flash->write(write_address, (uint8_t*)&entry, sizeof(entry));
            write_address += sizeof(entry);
            while(left)
            {
                uint32_t copy_size = left;
                if(copy_size > 32) copy_size = 32;
                flash->read(read_address, buffer, copy_size);
                flash->write(write_address, buffer, copy_size);
                read_address += copy_size;
                write_address += copy_size;
                left -= copy_size;
            }
            keys_left--;
        }
        else
            read_address += storedLength(entry.keylen) + storedLength(entry.vallen);
    }

    header.id = (header.id == STORE_ID_B) ? STORE_ID_A : STORE_ID_B;
    header.count++;
    flash->write(write_header_address, (uint8_t*)&header, sizeof(header));
    reclaim = 0;
    first_key_address = write_header_address + sizeof(header);
    block_address = write_header_address;
    free_key_address = write_address;

    return true;
}

//delete the store
bool FlashStore::erase()
{
    if(!loaded) return false;
    if(sectorSize() == 0) return false;

    for(int i=start_address; i<start_address+header.size*2; i+=sectorSize())
    {
        flash->eraseSector(i);
    }
    loaded = false;
}
