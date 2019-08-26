/*
  UBlox.h - Class definitions that provide a ublox driver.

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

#include "../Network.h"
#include "../modem/Modem.h"

#ifndef UBLOX_SOCKET_COUNT
#define UBLOX_SOCKET_COUNT 7
#endif

#ifndef UBLOX_SOCKET_WR_BUFFER_SIZE
#define UBLOX_SOCKET_WR_BUFFER_SIZE 1024
#endif

#define UBLOX_MODEL_SIZE 16

typedef struct {
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    int8_t  tzquarter;
}timestamp_tz;

typedef struct {
    char sender[21];
    timestamp_tz timestamp;
    char message[161];
}sms_event;

typedef struct {
    timestamp_tz timestamp;
    char lat[16];
    char lon[16];
    int altitude;
    int uncertainty;
}location_event;

typedef struct {
    char host[16];
    int port;
}ip_endpoint;

typedef struct {
    int socket;
    ip_endpoint remote;
    ip_endpoint local;
    int listener;
}socket_accept_event;

typedef union {
    int sms_index;
    socket_accept_event accept;
    location_event location;
}ublox_event_content;

typedef enum {
    UBLOX_EVENT_NONE = 0,               //NULL
    UBLOX_EVENT_SMS_RECEIVED = 1,       //sms_event
    UBLOX_EVENT_FORCED_DISCONNECT = 2,  //NULL
    UBLOX_EVENT_NETWORK_TIME_UPDATE = 3,//NULL
    UBLOX_EVENT_SOCKET_ACCEPT = 4,      //connection_event
    UBLOX_EVENT_LOCATION_UPDATE = 5,    //location_event
    UBLOX_EVENT_NETWORK_REGISTERED = 6, //NULL
    UBLOX_EVENT_NETWORK_UNREGISTERED = 7, //NULL
    UBLOX_EVENT_CONNECTED = 8,          //NULL
}ublox_event_id;

typedef enum {
    UBLOX_CONN_REGISTERED = 0,
    UBLOX_CONN_CONNECTED = 1,
    UBLOX_CONN_ERR_SIM = 3,
    UBLOX_CONN_ERR_UNREGISTERED = 4,
    UBLOX_CONN_ERR_SIGNAL = 5,
    UBLOX_CONN_ERR_CONNECT = 12,
    UBLOX_CONN_ERR_OFF = 15, //powered off or never initialized
}ublox_connection;

typedef enum {
    UBLOX_STATE_INIT,
    UBLOX_STATE_OFF,
    UBLOX_STATE_CHECK_SIM,
    UBLOX_STATE_NO_SIM,
    //UBLOX_STATE_SIM_READY,
    UBLOX_STATE_UNREGISTERED,
    UBLOX_STATE_REGISTERED,
    UBLOX_STATE_CONNECTING,
    UBLOX_STATE_CONNECTED,
}ublox_state;

typedef enum {
    SOCKET_TYPE_NONE,
    SOCKET_TYPE_LISTEN,
    SOCKET_TYPE_ACTIVE,
}socket_type;

class UBlox : public Network, public URCReceiver {
public:
    UBlox();
    virtual void init(NetworkEventHandler &handler, Modem &m);
    virtual void onURC(const char* urc);

    int getConnectionStatus();
    bool isInitialized();
    bool isReady();
    bool isRegistered();
    virtual bool isConnected();
    int getSignalStrength();

    virtual void powerUp();
    virtual void powerDown(bool soft=true);

    bool getLocation(int mode, int sensor, int response_type, int timeout, int accuracy, int num_hypothesis=1);

    bool deleteSMS(int location);
    bool readSMS(int location, sms_event &smsread);
    int getNumSMS();
    int getSlotsSMS();

    void pollEvents();

    uint32_t timeoutCount() {return modem->timeoutCount();}

    const char* getModel();

    int open(int port);
    int open(const char* port);
    int open(const char* host, int port);
    int open(const char* host, const char* port);
    bool write(int socket, const uint8_t* content, int length);
    void flush(int socket);
    int read(int socket, int numbytes, uint8_t *buffer) {return read(socket, numbytes, buffer, 10000);}
    int read(int socket, int numbytes, uint8_t *buffer, uint32_t timeout) {return read(socket, numbytes, buffer, timeout, false);}
    int read(int socket, int numbytes, uint8_t *buffer, uint32_t timeout, bool hex);
    bool close(int socket);

    int getIMSI(char *id);
    int getICCID(char *id);
    bool isNetworkTimeAvailable();
    bool getNetworkTime(timestamp_tz& ts);
    bool httpGet(const char* url, int port, const char* response, const char* user, const char* pass);

    //UBlox File System
    int filesize(const char *filename);
    int readFile(const char* filename, int offset, void* buffer, int size);
    int readFileLine(const char* filename, int offset, char* buffer, int size);

    const char* query(const char* cmd, uint32_t timeout=500, uint32_t retries=0);
    const char* command(const char* cmd, uint32_t timeout=500, uint32_t retries=0);
    const char* set(const char* cmd, const char* value, uint32_t timeout=500, uint32_t retries=0);

    static uint8_t invertDecimal(const char *dec);
    static uint8_t invertHex(const char *hex);
    static uint8_t convertField(const char* field);

protected:
    typedef struct {
        int id;
        int bytes_available;
        socket_type type;
    }ublox_socket;

    virtual void wait(uint32_t ms)=0;
    virtual void holdReset()=0;
    virtual void releaseReset()=0;
    virtual void toggleReset()=0;
    virtual uint32_t modemResetTime() { return 4;}
    virtual void debug(const char* msg){}
    virtual void debugln(const char* msg){}
    virtual void debug(int i){}
    virtual void debugln(int i){}

    void state_init();
    void state_check_sim();
    void state_unregistered();
    void state_registered();

    bool checkRegistered();
    void setRegistered(bool reg);
    bool initModem(int delay_seconds=1);
    bool startswith(const char* a, const char* b);

    void setHexMode(bool hex);

    int nextSocket();
    int mapSocket(int socket);
    int _socketState(int socketnum);
    int _available(int socketnum);
    bool _close(int socketnum);

    void rev_octet(char*& dst, const char* src);
    char gsm7toascii(char c, bool esc);
    void convert7to8bit(char* dst, const char* src, int num_chars);
    bool parse_sms_pdu(const char* fullpdu, sms_event &parsed_sms);

    bool uhttp(int profile, int opcode, const char* value);
    bool uhttp(int profile, int opcode, int value);

    Modem *modem;

    bool rn4_series;

    ublox_state state;

    char pdu[284];
    sms_event sms;

    ublox_socket sockets[UBLOX_SOCKET_COUNT];
    uint8_t write_buffer[UBLOX_SOCKET_WR_BUFFER_SIZE];
    int write_count;
    uint32_t write_id;

    int socket_id;
    int httpGetFlag;

    uint16_t num_sms;
    uint16_t slots_sms;
    bool networkTimeValid;

    char model[UBLOX_MODEL_SIZE];
};
