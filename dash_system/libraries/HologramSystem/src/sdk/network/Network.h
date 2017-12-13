/*
  Network.h - Abstract class definitions that provide a common network
  interface.

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

#include <cstdint>
#include <cstddef>
#include <cstdint>

class NetworkEventHandler {
public:
    virtual void onNetworkEvent(uint32_t id, const void* content)=0;
    virtual void onPowerUp()=0;
    virtual void onPowerDown()=0;
};

class Network {
public:
    virtual void init(NetworkEventHandler &handler);
    virtual bool isConnected()=0;
    virtual int getConnectionStatus()=0;
    virtual int getSignalStrength()=0;

    virtual void powerUp()=0;
    virtual void powerDown(bool soft=true)=0;

    virtual void pollEvents()=0;

    virtual const char* getModel()=0;

    virtual int open(int port)=0;
    virtual int open(const char* port)=0;
    virtual int open(const char* host, int port)=0;
    virtual int open(const char* host, const char* port)=0;
    virtual bool write(int socket, const char* content);
    virtual bool write(int socket, const uint8_t* content, int length)=0;
    virtual void flush(int socket)=0;
    virtual int read(int socket, int numbytes, uint8_t *buffer)=0;
    virtual int read(int socket, int numbytes, uint8_t *buffer, uint32_t timeout)=0;
    virtual int read(int socket, int numbytes, uint8_t *buffer, uint32_t timeout, bool hex)=0;
    virtual bool close(int socket)=0;
protected:
    NetworkEventHandler *eventHandler;
};
