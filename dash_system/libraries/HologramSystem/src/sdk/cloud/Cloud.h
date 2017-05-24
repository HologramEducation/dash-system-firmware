/*
  Cloud.h - Class definitions that provide access to Hologram Cloud

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

#include "../network/Network.h"
#include "../Authentication.h"

class Cloud : public AuthenticationWriter {
public:
    void init(Network &network, AuthenticationMethod method);
    bool sendMessage(const char* content);
    bool sendMessage(const char* content, const char* topic);
    bool sendMessage(const char* content, const char* topics[], uint32_t numtopics);
    bool sendMessage(const uint8_t* content, uint32_t length);
    bool sendMessage(const uint8_t* content, uint32_t length, const char* topic);
    bool sendMessage(const uint8_t* content, uint32_t length, const char* topics[], uint32_t numtopics);

    void acknowledgeAccept(int socket);

    int listen(int port);

    virtual bool write(int id, const char* content);
    virtual bool write(int id, const uint8_t* content, int length);
    virtual bool write(int id, uint8_t byte);
    virtual bool write(int id, char c);

protected:
    Network *network;
    Authentication *auth;

    virtual const char* getID(){return "";}
    virtual const char* getKey(){return "";}
    virtual const char* getHost(){return "";}
    virtual int getPort(){return 0;}
    virtual uint32_t getSeconds(){return 0;}
};
