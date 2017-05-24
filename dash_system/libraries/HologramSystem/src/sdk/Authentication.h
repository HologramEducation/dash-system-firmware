/*
  Authentication.h - Abstract class definitions that provide authorization
  functions for accessing the Hologram Cloud.

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

#include <cstring>
#include <cstdint>
#include <cstddef>
#include <cstdint>

typedef enum {
    AUTH_NONE,
    AUTH_TOTP,
}AuthenticationMethod;

class AuthenticationWriter {
public:
    virtual bool write(int id, const char* content)=0;
    virtual bool write(int id, const uint8_t* content, int length)=0;
    virtual bool write(int id, uint8_t byte)=0;
    virtual bool write(int id, char c)=0;
};

class Authentication {
public:
    virtual int generateAuth(const uint8_t *msg,
                             int length,
                             const char *id,
                             const char *key,
                             uint32_t clockTime,
                             char *authout)=0;

    int generateAuth(const char *msg,
                     const char *id,
                     const char *key,
                     uint32_t clockTime,
                     char *authout)
    {return generateAuth((const uint8_t*)msg, strlen(msg), id, key, clockTime, authout);}

    virtual void writeAuth(const uint8_t *msg,
                           int length,
                           const char *id,
                           const char *key,
                           uint32_t clockTime,
                           AuthenticationWriter &writer,
                           int socket)=0;

    void writeAuth(const char *msg,
                   const char *id,
                   const char *key,
                   uint32_t clockTime,
                   AuthenticationWriter &writer,
                   int socket)
    {return writeAuth((const uint8_t*)msg, (int)strlen(msg), id, key, clockTime, writer, socket);}

    virtual bool verifyToken(const uint8_t *msg,
                             int length,
                             const char *id,
                             const char *key,
                             uint32_t clockTime,
                             const char* token)=0;

    bool verifyToken(const char *msg,
                     const char *id,
                     const char *key,
                     uint32_t clockTime,
                     const char* token)
    {return verifyToken((const uint8_t*)msg, (int)strlen(msg), id, key, clockTime, token);}

    virtual int generateToken(const uint8_t *msg,
                              int length,
                              const char *id,
                              const char *key,
                              uint32_t clockTime,
                              char* token)=0;

    int generateToken(const char *msg,
                      const char *id,
                      const char *key,
                      uint32_t clockTime,
                      char* token)
    {return generateToken((const uint8_t*)msg, (int)strlen(msg), id, key, clockTime, token);}

    virtual int generatePassword(const char *id,
                                 const char *key,
                                 uint32_t clockTime,
                                 char* password)=0;

    virtual const char* validateCommand(const char* command,
                                        const char *id,
                                        const char *key,
                                        uint32_t clockTime)=0;
};

class AuthenticationFactory {
public:
    static Authentication* getAuthentication(AuthenticationMethod method);
private:
    AuthenticationFactory(){}
};
