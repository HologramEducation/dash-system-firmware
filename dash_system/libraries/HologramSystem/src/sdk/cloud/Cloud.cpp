/*
  Cloud.cpp - Class definitions that provide access to Hologram Cloud

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
#include "Cloud.h"
#include "variant.h"

void Cloud::init(Network &network, AuthenticationMethod method) {
    this->network = &network;
    this->auth = AuthenticationFactory::getAuthentication(method);
}

bool Cloud::sendMessage(const char* content) {
    return sendMessage((const uint8_t*)content, strlen(content), NULL, 0);
}

bool Cloud::sendMessage(const char* content, const char* topic) {
    const char* topics[] = {topic};
    return sendMessage((const uint8_t*)content, strlen(content), topics, 1);
}

bool Cloud::sendMessage(const char* content, const char* topics[], uint32_t numtags) {
    return sendMessage((const uint8_t*)content, strlen(content), topics, numtags);
}

bool Cloud::sendMessage(const uint8_t* content, uint32_t length) {
    return sendMessage(content, length, NULL, 0);
}

bool Cloud::sendMessage(const uint8_t* content, uint32_t length, const char* topic) {
    const char* topics[] = {topic};
    return sendMessage(content, length, topics, 1);
}

static const uint8_t metadata_version = 0x01;

bool Cloud::sendMessage(const uint8_t* content, uint32_t length, const char* topics[], uint32_t numtags) {
    if(!network->isConnected())
        return false;

    uint8_t response[3] = {0,0,0};
    int socket = network->open(getHost(), getPort());
    auth->writeAuth(content, length, getID(), getKey(), getSeconds(), *this, socket);
    network->write(socket, " M");
    network->write(socket, &metadata_version, 1);
    network->write(socket, "dash-");
    network->write(socket, network->getModel());
    network->write(socket, "-");
    network->write(socket, FIRMWARE_VERSION_STRING);
    network->write(socket, "\n");
    for(int i=0; i<numtags; i++) {
        network->write(socket, "T");
        network->write(socket, topics[i]);
        network->write(socket, "\n");
    }
    network->write(socket, "B");
    const uint8_t escapechar = '\\';
    for(int i=0; i<length; i++) {
        if(content[i] == 0) {
            network->write(socket, "\\0");
        } else if(content[i] == escapechar) {
            network->write(socket, "\\\\");
        } else {
            network->write(socket, &content[i], 1);
        }
    }
    network->write(socket, response, 2);
    network->flush(socket);
    int numread = network->read(socket, 2, response);
    response[2] = 0;
    network->close(socket);
    return((numread == 2) && (strcmp("00", (const char*)response) == 0));
}

int Cloud::listen(int port) {
    if(!network->isConnected())
        return false;
    return network->open(port);
}

void Cloud::acknowledgeAccept(int socket) {
    network->write(socket, "OK");
    network->flush(socket);
}

bool Cloud::write(int id, const char* content) {
    return network->write(id, content);
}

bool Cloud::write(int id, const uint8_t* content, int length) {
    return network->write(id, content, length);
}

bool Cloud::write(int id, uint8_t byte) {
    return network->write(id, &byte, 1);
}

bool Cloud::write(int id, char c) {
    return network->write(id, (uint8_t*)&c, 1);
}
