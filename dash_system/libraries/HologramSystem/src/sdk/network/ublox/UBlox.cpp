/*
  UBlox.cpp - Class definitions that provide a ublox driver.

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
#include "UBlox.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

#define MAX_READ_LEN 32

void UBlox::init(NetworkEventHandler &handler, Modem &m) {
    Network::init(handler);
    modem = &m;
    //connectStatus = UBLOX_CONN_ERR_OFF;
    state = UBLOX_STATE_INIT;
    networkTimeValid = false;
    for(int i=0; i<UBLOX_SOCKET_COUNT; i++) {
        sockets[i].bytes_available = 0;
        sockets[i].type = SOCKET_TYPE_NONE;
        sockets[i].id = 0;
    }
}

void UBlox::powerUp() {
    if(state == UBLOX_STATE_OFF) {
        eventHandler->onPowerUp();
        state = UBLOX_STATE_INIT;
        toggleReset();
    }
}

void UBlox::powerDown(bool soft) {
    state = UBLOX_STATE_OFF;
    networkTimeValid = false;
    if(soft) {
        if(modem->command("", 200) == MODEM_OK) {
            if(modem->command("+CPWROFF", 40000) == MODEM_OK) {
                //return;
            }
        }
    }
    eventHandler->onPowerDown();
}

void UBlox::state_init() {
    static int retries = 100; //depends on modem type???

    if(modem->command("", 100) == MODEM_OK) {
        retries = 100;
        modem->command("&K0"); //flow control off
        modem->command("E0"); //echo off
        modem->set("+CMEE", "2"); //set verbose error codes
        loadModel();
        state = UBLOX_STATE_CHECK_SIM;
    } else {
        if(retries-- <= 0) {
            toggleReset();
            retries = 100;
        }
    }
}

void UBlox::state_check_sim() {
    static int cpin_count = 50;
    if(modem->query("+CPIN", "+CPIN: READY") == MODEM_OK) {
        state = UBLOX_STATE_UNREGISTERED;
        cpin_count = 50;
        modem->set("+CTZU", "1"); //time/zone sync
        modem->set("+CTZR", "1"); //time/zone URC
        modem->set("+CPMS", "\"ME\",\"ME\",\"ME\"");
        modem->set("+CMGF", "0"); //SMS PDU format
        modem->set("+CNMI", "2,1"); //SMS New Message Indication
        modem->set("+UPSD", "0,1,\"hologram\"", 3000);
        modem->set("+UPSD", "0,7,\"0.0.0.0\"", 3000);
        modem->set("+CREG", "2");
        modem->set("+CGREG", "2");

        for(int i=0; i<UBLOX_SOCKET_COUNT; i++) {
            //get socket states
            switch(_socketState(i)) {
            case 0: //CLOSED
                break;
            case 1: //LISTEN
                sockets[i].type = SOCKET_TYPE_LISTEN;
                break;
            default:
                sockets[i].type = SOCKET_TYPE_ACTIVE;
                _available(i); //load bytes_available
                break;
            }
        }
        getNumSMS();
    }
    else if(--cpin_count <= 0) {
        if((state == UBLOX_STATE_CHECK_SIM) &&
           (strcmp("+CME ERROR: SIM not inserted", modem->lastResponse()) == 0)) {
            state = UBLOX_STATE_NO_SIM;
            //notify?
        }
    }
}

void UBlox::state_unregistered() {
    bool reg = false;
    bool greg = false;
    int n = 0;
    int stat = 0;
    if(modem->query("+CREG") == MODEM_OK) {
        if(sscanf(modem->lastResponse(), "+CREG: %d,%d", &n, &stat) == 2) {
            reg = (stat == 1) || (stat == 5);
        } else if(sscanf(modem->lastResponse(), "+CREG: %d", &stat) == 1) {
            reg = (stat == 1) || (stat == 5);
        }
    }
    if(!reg) {
        if(modem->query("+CGREG") == MODEM_OK) {
            if(sscanf(modem->lastResponse(), "+CGREG: %d,%d", &n, &stat) == 2) {
                greg = (stat == 1) || (stat == 5);
            } else if(sscanf(modem->lastResponse(), "+CGREG: %d", &stat) == 1) {
                greg = (stat == 1) || (stat == 5);
            }
        }
    }

    setRegistered(reg || greg);
}

void UBlox::state_registered() {
    if((modem->set("+UPSND", "0,8", "+UPSND: 0,8,1") == MODEM_OK) &&
       (modem->set("+UPSND", "0,0") == MODEM_OK)) {
        state = UBLOX_STATE_CONNECTED;
        eventHandler->onNetworkEvent(UBLOX_EVENT_CONNECTED, NULL);
    } else {
        state = UBLOX_STATE_CONNECTING;
        modem->asyncSet("+UPSDA", "0,3", 180000);
    }
}

void UBlox::pollEvents() {
    modem->checkURC();
    switch(state) {
        case UBLOX_STATE_INIT:
            state_init();
            break;
        case UBLOX_STATE_CHECK_SIM:
        case UBLOX_STATE_NO_SIM:
            state_check_sim();
            break;
        case UBLOX_STATE_UNREGISTERED:
            state_unregistered();
            break;
        case UBLOX_STATE_REGISTERED:
            state_registered();
            break;
        case UBLOX_STATE_CONNECTING:
            if(modem->asyncStatus() != MODEM_BUSY) {
                state = UBLOX_STATE_REGISTERED;
                state_registered();
            }
            break;
        case UBLOX_STATE_CONNECTED:
            modem->command("", 100);
            break;
    }
    if(isInitialized()) {
        if(modem->timeoutCount() > 10 && modem->asyncStatus() != MODEM_BUSY) {
            if(modem->command("", 100) != MODEM_OK) {
                state = UBLOX_STATE_OFF;
                powerUp();
            }
        }
    }
}

bool UBlox::startswith(const char* a, const char* b) {
  return strncmp(a, b, strlen(b)) == 0;
}

void UBlox::setRegistered(bool reg) {
    ublox_event_id event = UBLOX_EVENT_NONE;

    if(reg && state == UBLOX_STATE_UNREGISTERED) {
        //now registered
        event = UBLOX_EVENT_NETWORK_REGISTERED;
        state = UBLOX_STATE_REGISTERED;
        networkTimeValid = true;
    } else if(!reg && state > UBLOX_STATE_UNREGISTERED) {
        //now unregistered
        event = UBLOX_EVENT_NETWORK_UNREGISTERED;
        state = UBLOX_STATE_UNREGISTERED;
    }

    if(event != UBLOX_EVENT_NONE) {
        eventHandler->onNetworkEvent(event, NULL);
    }
}

bool UBlox::isRegistered() {
    return state > UBLOX_STATE_UNREGISTERED;
}

int UBlox::getConnectionStatus() {
    switch(state) {
        case UBLOX_STATE_CONNECTED: return UBLOX_CONN_CONNECTED;
        case UBLOX_STATE_CONNECTING:
        case UBLOX_STATE_REGISTERED: return UBLOX_CONN_REGISTERED;
        case UBLOX_STATE_NO_SIM: return UBLOX_CONN_ERR_SIM;
        case UBLOX_STATE_OFF: return UBLOX_CONN_ERR_OFF;
        case UBLOX_STATE_UNREGISTERED: return UBLOX_CONN_ERR_SIGNAL;
        default: return UBLOX_CONN_ERR_UNREGISTERED;
    }
}

bool UBlox::isInitialized() {
    return state > UBLOX_STATE_INIT;
}

bool UBlox::isReady() {
    return state >= UBLOX_STATE_UNREGISTERED && state != UBLOX_STATE_CONNECTING;
}

bool UBlox::isConnected() {
    return state == UBLOX_STATE_CONNECTED;
}

int UBlox::getSignalStrength() {
    int rssi = 99;
    int qual = 0;
    if(isReady() && modem->command("+CSQ") == MODEM_OK) {
        sscanf(modem->lastResponse(), "+CSQ: %d,%d", &rssi, &qual);
    }
    return rssi;
}

int UBlox::getIMSI(char *imsi) {
    if(isReady() && modem->command("+CIMI") == MODEM_OK) {
        strcpy(imsi, modem->lastResponse());
        return strlen(imsi);
    }
    return 0;
}

int UBlox::getICCID(char *iccid) {
    if(isReady() && modem->command("+CCID") == MODEM_OK) {
        strcpy(iccid, &(modem->lastResponse()[7]));
        return strlen(iccid);
    }
    return 0;
}

void UBlox::loadModel() {
    if(isInitialized() && modem->command("+CGMM") == MODEM_OK) {
        strncpy(model, modem->lastResponse(), UBLOX_MODEL_SIZE);
    }
}

const char* UBlox::getModel() {
    if(model == NULL) {
        loadModel();
        if(model == NULL)
            return "-unknown";
    }
    return model;
}

bool UBlox::getLocation(int mode, int sensor, int response_type, int timeout, int accuracy, int num_hypothesis) {
    if(!isConnected()) return false;
    modem->startSet("+ULOC");
    modem->appendSet(mode);
    modem->appendSet(',');
    modem->appendSet(sensor);
    modem->appendSet(',');
    modem->appendSet(response_type);
    modem->appendSet(',');
    modem->appendSet(timeout);
    modem->appendSet(',');
    modem->appendSet(accuracy);
    modem->appendSet(',');
    modem->appendSet(num_hypothesis);
    return modem->completeSet(2000) == MODEM_OK;
}

uint8_t UBlox::convertField(const char* field) {
    char local[3] = {0,0,0};
    memcpy(local, field, 2);
    return atoi(local);
}

bool UBlox::isNetworkTimeAvailable() {
    if(networkTimeValid) return true;
    if(isRegistered()) return true;
}

bool UBlox::getNetworkTime(timestamp_tz& ts) {
    if(!isNetworkTimeAvailable() || modem->query("+CCLK") != MODEM_OK) {
        return false;
    }
    //00000000001111111111222222222
    //01234567890123456789012345678
    //+CCLK: "yy/MM/dd,hh:mm:ss+zz"
    const char* r = modem->lastResponse();
    ts.year = convertField(&r[8]);
    ts.month = convertField(&r[11]);
    ts.day = convertField(&r[14]);
    ts.hour = convertField(&r[17]);
    ts.minute = convertField(&r[20]);
    ts.second = convertField(&r[23]);
    ts.tzquarter = convertField(&r[26]);
    if(r[25] == '-')
        ts.tzquarter *= -1;

    if(ts.year == 4) {
        if(getSignalStrength() == 99)
            return false;
    }
    return true;
}

int UBlox::getNumSMS() {
    num_sms = 0;
    slots_sms = 0;
    if(isReady() && modem->query("+CPMS") == MODEM_OK) {
        int inuse, slots;
        if(sscanf(modem->lastResponse(), "+CPMS: \"ME\",%d,%d", &inuse, &slots) == 2) {
            num_sms = inuse;
            slots_sms = slots;
        }
    }
    return num_sms;
}

int UBlox::getSlotsSMS() {
    if(slots_sms == 0) {
        getNumSMS();
    }
    return slots_sms;
}

bool UBlox::deleteSMS(int location) {
    if(!isReady()) return false;

    modem->startSet("+CMGD");
    modem->appendSet(location);
    return (modem->completeSet() == MODEM_OK);
}

bool UBlox::readSMS(int location, sms_event &smsread) {
    if(!isReady()) return false;

    modem->startSet("+CMGR");
    modem->appendSet(location);
    if(modem->completeSet() == MODEM_OK) {
        if(parse_sms_pdu(modem->lastResponse(), smsread)) {
            return true;
        } else {
            deleteSMS(location);
        }
    }
    return false;
}

void UBlox::rev_octet(char*& dst, const char* src) {
  *dst++ = src[1];
  *dst++ = src[0];
}

char UBlox::gsm7toascii(char c, bool esc)
{
    if(esc) {
        switch(c) {
            case 10: return 10;
            case 20: return '^';
            case 40: return '{';
            case 41: return '}';
            case 47: return '\\';
            case 60: return '[';
            case 61: return '~';
            case 62: return ']';
            case 64: return '|';
            default: return ' ';
        }
    }

    if(c >= 18 && c < 27) return ' ';
    switch(c) {
        case 1:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 11:
        case 12:
        case 14:
        case 15:
        case 16:
        case 28:
        case 29:
        case 30:
        case 31:
        case 36:
        case 64:
        case 91:
        case 92:
        case 93:
        case 94:
        case 95:
        case 96:
        case 123:
        case 124:
        case 125:
        case 126:
        case 127:
            return ' ';
        case 0: return '@';
        case 2: return '$';
        case 17: return '_';
        default: return c;
    }
}

void UBlox::convert7to8bit(char* dst, const char* src, int num_chars) {
    int offset = 0;
    int last = 0;
    int current = 0;
    char bytebuf[3];
    bool inescape = false;

    while(num_chars--) {
        last = current;
        if(offset < 7) {
            strncpy(bytebuf, src, 2);
            sscanf(bytebuf, "%x", &current);
            src += 2;
        }

        *dst = 0;
        *dst = last >> (8-offset);
        *dst |= current << offset;
        *dst &= 0x7F;
        if(*dst == 27 && !inescape) { //ESC
            inescape = true;
        } else {
            *dst = gsm7toascii(*dst, inescape);
            inescape = false;
            dst++;
        }
        offset++;
        if(offset == 8) offset = 0;
    }
    *dst = 0;
}

uint8_t UBlox::invertDecimal(const char *dec) {
    uint8_t d = Modem::convertHex(dec[1]) * 10;
    d += Modem::convertHex(dec[0]);
    return d;
}

uint8_t UBlox::invertHex(const char *hex) {
    uint8_t h = Modem::convertHex(hex[1]) << 4;
    h |= Modem::convertHex(hex[0]);
    return h;
}

bool UBlox::parse_sms_pdu(const char* fullpdu, sms_event &parsed_sms) {
    const char* p = fullpdu;
    int smsc_len = Modem::convertHex(p);        //SMSC Length
    p += 2+(smsc_len*2);                        //skip SMSC Content
    if(p[1] & 0x3 != 0) return false;           //SMS-DELIVER
    p += 2;
    int sender_len = Modem::convertHex(p);      //Sender length
    p += 2;
    uint8_t sender_type = Modem::convertHex(p); //Sender Number Type
    p += 2;
    int sender_read = sender_len;
    if(sender_read & 1) sender_read++;

    if((sender_type & 0x50) == 0x50) {
        convert7to8bit(parsed_sms.sender, p, sender_len*4/7);
    } else {
        char *smsdst = parsed_sms.sender;
        for(int i=0; i<sender_read; i+=2) {
            rev_octet(smsdst, &p[i]);
        }
    }
    parsed_sms.sender[sender_len] = 0;
    p += sender_read;
    if(strncmp(p, "0000", 4) != 0) return false; //protocol/encoding
    p += 4;
    parsed_sms.timestamp.year = invertDecimal(&p[0]);
    parsed_sms.timestamp.month = invertDecimal(&p[2]);
    parsed_sms.timestamp.day = invertDecimal(&p[4]);
    parsed_sms.timestamp.hour = invertDecimal(&p[6]);
    parsed_sms.timestamp.minute = invertDecimal(&p[8]);
    parsed_sms.timestamp.second = invertDecimal(&p[10]);
    uint8_t tz = invertHex(&p[12]);
    parsed_sms.timestamp.tzquarter = (int8_t)(((tz>>4)&0x07)*10 + (tz&0x0F));
    if((tz >> 7) == 1) {
        parsed_sms.timestamp.tzquarter *= -1;
    }
    p += 14;
    int msg_len = Modem::convertHex(p);
    p += 2;
    //Convert 7-bit to chars
    convert7to8bit(parsed_sms.message, p, msg_len);
    return true;
}

void UBlox::onURC(const char* urc) {
    if(startswith(urc, "+UUSORD: ")) {
        int sock, len;
        if(sscanf(urc, "+UUSORD: %d,%d", &sock, &len) == 2) {
            //TODO confirm socket state variables
            sockets[sock].bytes_available += len;
        }
    } else if(startswith(urc, "+UUSOCL: ")) {
        int sock;
        if(sscanf(urc, "+UUSOCL: %d", &sock) == 1) {
            sockets[sock].id = 0;
            sockets[sock].type = SOCKET_TYPE_NONE;
            //TODO: handle LISTEN vs ACTIVE?
            sockets[sock].bytes_available = 0;
        }
    } else if(startswith(urc, "+CMTI: ")) {
        int addr=0;
        char mem[8];
        if(sscanf(urc, "+CMTI: \"%[^\"]\",%d", mem, &addr) == 2) {
            eventHandler->onNetworkEvent(UBLOX_EVENT_SMS_RECEIVED, &addr);
        }
    } else if(startswith(urc, "+UMWI: ")) {
        //TODO startup messages
    } else if(strcmp(urc, "+UUPSDD: 0") == 0) {
        setRegistered(false);
        for(int i=0; i<UBLOX_SOCKET_COUNT; i++) {
            //any active sockets are closed?
            //_close(socketnum) vs close(socket)
        }
        eventHandler->onNetworkEvent(UBLOX_EVENT_FORCED_DISCONNECT, NULL);
    } else if(startswith(urc, "+CREG: ")) {
        int stat = 0;
        if(sscanf(urc, "+CREG: %d,", &stat) == 1) {
            setRegistered((stat == 1) || (stat == 5));
        }
    } else if(startswith(urc, "+CGREG: ")) {
        int stat = 0;
        if(sscanf(urc, "+CGREG: %d,", &stat) == 1) {
            setRegistered((stat == 1) || (stat == 5));
        }
    } else if(startswith(urc, "+UUHTTPCR: ")) {
        int flag;
        if(sscanf(urc, "+UUHTTPCR: 0,1,%d", &flag) == 1) {
            httpGetFlag = flag;
        }
    } else if(startswith(urc, "+CTZV: ")) {
        //TODO Timezone updated, ublox RTC set
        eventHandler->onNetworkEvent(UBLOX_EVENT_NETWORK_TIME_UPDATE, NULL);
    } else if(startswith(urc, "+UUSOLI: ")) {
        //+UUSOLI: <socket>,<"ip_address">,<port>,<listening_socket>,<"local_ip_address">,<listenting_port>
        int socketnum;
        int server;
        socket_accept_event event;
        if(sscanf(urc, "+UUSOLI: %d,\"%[^\"]\",%d,%d,\"%[^\"]\",%d", &socketnum, event.remote.host, &event.remote.port, &server, event.local.host, &event.local.port) == 6) {
            //new inbound socket
            sockets[socketnum].bytes_available = 0;
            sockets[socketnum].id = nextSocket();
            sockets[socketnum].type = SOCKET_TYPE_ACTIVE;

            event.socket = sockets[socketnum].id;
            event.listener = sockets[server].id;

            eventHandler->onNetworkEvent(UBLOX_EVENT_SOCKET_ACCEPT, &event);
        }
    } else if(startswith(urc, "+UULOC: ")) {
        //+UULOC: 13/04/2011,09:54:51.000,45.6334520,13.0618620,49,1
        location_event event;
        char numbuf[16];
        const char *pstr = &urc[8];

        event.timestamp.tzquarter = 0;
        event.timestamp.day = atoi(pstr);
        pstr = strchr(pstr, '/') + 1;
        event.timestamp.month = atoi(pstr);
        pstr = strchr(pstr, '/') + 1;
        uint32_t year = atoi(pstr);
        event.timestamp.year = year >= 2000 ? (year-2000) : year;
        pstr = strchr(pstr, ',') + 1;
        event.timestamp.hour = atoi(pstr);
        pstr = strchr(pstr, ':') + 1;
        event.timestamp.minute = atoi(pstr);
        pstr = strchr(pstr, ':') + 1;
        event.timestamp.second = atoi(pstr);
        pstr = strchr(pstr, ',') + 1;
        char *pend = strchr(pstr, ',');
        memcpy(event.lat, pstr, pend-pstr);
        event.lat[pend-pstr] = 0;
        pstr = pend+1;
        pend = strchr(pstr, ',');
        memcpy(event.lon, pstr, pend-pstr);
        event.lon[pend-pstr] = 0;
        pstr = pend+1;
        event.altitude = atoi(pstr);
        pstr = strchr(pstr, ',') + 1;
        event.uncertainty = atoi(pstr);

        eventHandler->onNetworkEvent(UBLOX_EVENT_LOCATION_UPDATE, &event);
    } else {
        debug("Unknown URC: ");
        debugln(urc);
    }
}

int UBlox::nextSocket() {
    return ++socket_id;
}

int UBlox::mapSocket(int socket) {
    for(int i=0; i<UBLOX_SOCKET_COUNT; i++) {
        if(sockets[i].id == socket)
            return i;
    }
    return -1;
}

int UBlox::open(const char* port) {
    return open(atoi(port));
}

int UBlox::open(int port) {
    if(!isConnected()) return false;

    int socketnum = -1;

    //only 1 listener at a time
    for(int i=0; i<UBLOX_SOCKET_COUNT; i++) {
        if(sockets[i].type == SOCKET_TYPE_LISTEN) {
            _close(i);
            break;
        }
    }

    if(modem->set("+USOCR", "6") == MODEM_OK) {
        if(sscanf(modem->lastResponse(), "+USOCR: %d", &socketnum) == 1) {
            sockets[socketnum].id = nextSocket();
            sockets[socketnum].type = SOCKET_TYPE_LISTEN;
            sockets[socketnum].bytes_available = 0;
            modem->startSet("+USOLI");
            modem->appendSet(socketnum);
            modem->appendSet(",");
            modem->appendSet(port);
            if(modem->completeSet(20000) == MODEM_OK) {
                return sockets[socketnum].id;
            }
            else
                _close(socketnum);
        }
    }
    return 0;
}

int UBlox::open(const char* host, const char* port) {
    return open(host, atoi(port));
}

int UBlox::open(const char* host, int port) {
    if(!isConnected()) return false;

    int socketnum = -1;
    if(modem->set("+USOCR", "6") == MODEM_OK) {
        if(sscanf(modem->lastResponse(), "+USOCR: %d", &socketnum) == 1) {
            sockets[socketnum].id = nextSocket();
            sockets[socketnum].type = SOCKET_TYPE_ACTIVE;
            sockets[socketnum].bytes_available = 0;
            modem->startSet("+USOCO");
            modem->appendSet(socketnum);
            modem->appendSet(",\"");
            modem->appendSet(host);
            modem->appendSet("\",");
            modem->appendSet(port);
            if(modem->completeSet(20000) == MODEM_OK) {
                return sockets[socketnum].id;
            }
            else
                _close(socketnum);
        }
    }
    return -1;
}

void UBlox::flush(int socket) {
    if(!isConnected()) return;

    if(write_id != socket) return;
    if(write_count == 0) return;

    int socketnum = mapSocket(socket);
    if((socketnum == -1) || (sockets[socketnum].type != SOCKET_TYPE_ACTIVE)) {
        write_id = 0;
        write_count = 0;
        return;
    }

    modem->checkURC();

    modem->startSet("+USOWR");
    modem->appendSet(socketnum);
    modem->appendSet(",");
    modem->appendSet(write_count);
    if(modem->intermediateSet('@', 10000) == MODEM_OK) {
        wait(50);
        for(int i=0; i<write_count; i++)
            modem->dataWrite(write_buffer[i]);

        if(modem->waitSetComplete(10000) == MODEM_OK) {
            int len = 0;
            int sock = 0;
            if(sscanf(modem->lastResponse(), "+USOWR: %d,%d", &sock, &len) == 2) {
                if((socketnum != sock) || (len != write_count)) {
                    debug("ERROR Writing to socket ");
                    debugln(socket);
                }
            } else {
                debug("ERROR lastResponse unexpected: ");
                debugln(modem->lastResponse());
            }
        }
    } else {
        debugln("ERROR could not write to socket!");
    }
    write_count = 0;
}

//loop and write at-most UBLOX_SOCKET_WR_BUFFER_SIZE bytes at a time
//have a local write buffer
//once full, or read or close is called, flush
bool UBlox::write(int socket, const uint8_t* content, int length) {
    if(!isConnected()) return false;

    if(socket != write_id) flush(write_id);

    int socketnum = mapSocket(socket);
    if(socketnum == -1) return false;
    if(sockets[socketnum].type != SOCKET_TYPE_ACTIVE) return false;

    write_id = socket;

    while(length) {
        int topush = UBLOX_SOCKET_WR_BUFFER_SIZE - write_count;
        if(length < topush) topush = length;
        memcpy(&(write_buffer[write_count]), content, topush);
        write_count += topush;
        if(write_count == UBLOX_SOCKET_WR_BUFFER_SIZE) {
            flush(socket);
        }
        content += topush;
        length -= topush;
    }

    return true;
}

void UBlox::setHexMode(bool hex) {
    modem->startSet("+UDCONF");
    modem->appendSet(1);
    modem->appendSet(",");
    modem->appendSet(hex ? 1 : 0);
    modem->completeSet();
}

int UBlox::_socketState(int socketnum) {
    if(!isReady()) return 0;

    modem->startSet("+USOCTL");
    modem->appendSet(socketnum);
    modem->appendSet(",10");
    if(modem->completeSet() == MODEM_OK) {
        int socket, paramid, sockstate;
        if(sscanf(modem->lastResponse(), "+USOCTL: %d,%d,%d", &socket, &paramid, &sockstate) == 3) {
            return sockstate;
        }
    }
    return 0; //assume closed
}

int UBlox::_available(int socketnum) {
    if(!isReady()) return -1;
    if(sockets[socketnum].type != SOCKET_TYPE_ACTIVE) return -1;

    modem->startSet("+USORD");
    modem->appendSet(socketnum);
    modem->appendSet(",0");
    if(modem->completeSet() == MODEM_OK) {
        int socket, avail;
        if(sscanf(modem->lastResponse(), "+USORD: %d,%d", &socket, &avail) == 2) {
            sockets[socketnum].bytes_available = avail;
            return avail;
        }
    }
    return -1;
}

//if hex mode, buffer must be numbytes*2
int UBlox::read(int socket, int numbytes, uint8_t *buffer, uint32_t timeout, bool hex) {
    if(!isReady()) return -1;
    if(numbytes == 0) return 0;
    int socketnum = mapSocket(socket);
    if(socketnum == -1) return -1;
    if(sockets[socketnum].type != SOCKET_TYPE_ACTIVE) return -1;

    if(write_id == socket) flush(write_id);

    int numread = 0;
    int numremain = numbytes;

    uint32_t startMillis = modem->msTick();
    do {
        modem->checkURC();
        if(sockets[socketnum].bytes_available) {
            int toread = sockets[socketnum].bytes_available;
            if(toread > numremain)
                toread = numremain;
            if(toread > MAX_READ_LEN/2)
                toread = MAX_READ_LEN/2;

            setHexMode(true);

            modem->startSet("+USORD");
            modem->appendSet(socketnum);
            modem->appendSet(",");
            modem->appendSet(toread);
            if(modem->completeSet() == MODEM_OK) {
                int socket, actual_read;
                if(sscanf(modem->lastResponse(), "+USORD: %d,%d,", &socket, &actual_read) == 2) {
                    const char* q = strchr(modem->lastResponse(), '"');
                    debug("READ: ");
                    debugln(q);
                    if(hex) {
                        memcpy(&buffer[numread*2], &q[1], actual_read*2);
                        numread += actual_read;
                    } else {
                        for(int i=0; i<actual_read; i++) {
                            buffer[numread++] = Modem::convertHex(&q[1+i*2]);
                        }
                    }
                    //sockets[socketnum].bytes_available -= actual_read;
                    sockets[socketnum].bytes_available = 0;
                    //numremain -= actual_read;
                    numremain = 0;
                }
            }
            setHexMode(false);
        }
    }while(sockets[socketnum].type == SOCKET_TYPE_ACTIVE && numremain && (modem->msTick() - startMillis < timeout));

    if(numread == 0 && sockets[socketnum].type != SOCKET_TYPE_ACTIVE)
        return -1;
    return numread;
}

bool UBlox::_close(int socketnum) {
    if(!isReady()) return false;
    if(sockets[socketnum].type != SOCKET_TYPE_NONE) {
        modem->startSet("+USOCL");
        modem->appendSet(socketnum);
        modem->completeSet(10000);
    }

    sockets[socketnum].id = 0;
    sockets[socketnum].type = SOCKET_TYPE_NONE;
    sockets[socketnum].bytes_available = 0;
    return true;
}

bool UBlox::close(int socket) {
    int socketnum = mapSocket(socket);
    if(socketnum == -1) return true;
    _close(socketnum);
}

bool UBlox::uhttp(int profile, int opcode, const char* value) {
    if(!isReady()) return false;
    modem->startSet("+UHTTP");
    modem->appendSet(profile);
    modem->appendSet(',');
    modem->appendSet(opcode);
    modem->appendSet(",\"");
    modem->appendSet(value);
    modem->appendSet('"');
    return (modem->completeSet(500, 10) == MODEM_OK);
}

bool UBlox::uhttp(int profile, int opcode, int value) {
    if(!isReady()) return false;
    modem->startSet("+UHTTP");
    modem->appendSet(profile);
    modem->appendSet(',');
    modem->appendSet(opcode);
    modem->appendSet(",");
    modem->appendSet(value);
    return (modem->completeSet(500, 10) == MODEM_OK);
}

bool UBlox::httpGet(const char* url, int port, const char* response,
                        const char* user, const char* pass) {
    if(!isConnected()) return false;
    //Server Name
    char* endofserver = strchr(url, '/');
    if(endofserver == NULL) return false;
    modem->startSet("+UHTTP");
    modem->appendSet("0,1,\"");
    modem->appendSet((uint8_t*)url, endofserver-url);
    modem->appendSet('"');
    if(modem->completeSet(500, 10) != MODEM_OK) return false;

    if(!uhttp(0, 2, user)) return false;
    if(!uhttp(0, 3, pass)) return false;
    if(!uhttp(0, 4, 1)) return false;
    if(!uhttp(0, 5, port)) return false;

    httpGetFlag = -1;
    modem->startSet("+UHTTPC");
    modem->appendSet("0,1,\"");
    modem->appendSet(endofserver);
    modem->appendSet("\",\"");
    modem->appendSet(response);
    modem->appendSet('"');
    if(modem->completeSet(5000) != MODEM_OK) return false;

    uint32_t startMillis = modem->msTick();
    while((httpGetFlag == -1) && (modem->msTick() - startMillis < 60000)) {
        modem->checkURC();
    }
    return httpGetFlag == 1;
}

int UBlox::filesize(const char *filename) {
    if(!isReady()) return -1;
    int filesize = -1;
    modem->startSet("+ULSTFILE");
    modem->appendSet("2,\"");
    modem->appendSet(filename);
    modem->appendSet('"');
    if(modem->completeSet(1000) == MODEM_OK) {
        if(sscanf(modem->lastResponse(), "+ULSTFILE: %d", &filesize) == 1) {
            return filesize;
        }
    }
    return -1;
}

int UBlox::readFile(const char* filename, int offset, void* buffer, int size) {
    //AT+URDBLOCK="<filename>",<offset>,<size>
    //only read at-most 32 bytes at a time from ublox
    if(!isReady()) return 0;
    if(offset < 0) return 0;
    int totalread = 0;
    int filenamelen = strlen(filename);
    char *pbuffer = (char*)buffer;
    char scratch[filenamelen < 12 ? 12 : filenamelen+1];

    while(size) {
        int toread = size;
        if(toread > 32) toread = 32;

        modem->startSet("+URDBLOCK");
        modem->appendSet('"');
        modem->appendSet(filename);
        modem->appendSet("\",");
        modem->appendSet(offset+totalread);
        modem->appendSet(',');
        modem->appendSet(toread);
        if(modem->intermediateSet('+', 10000) == MODEM_OK) {
            //+URDBLOCK: "<filename>",<numread>,"<content>"
            modem->rawRead(11, scratch);
            if(strcmp(scratch, "URDBLOCK: \"") != 0) break;

            modem->rawRead(filenamelen, scratch);
            if(strcmp(scratch, filename) != 0) break;

            modem->rawRead(2, scratch);
            if(strcmp(scratch, "\",") != 0) break;

            modem->rawRead(2, scratch);
            int numread = 0;
            if(scratch[0] < '0' || scratch[0] > '9') break;
            numread = scratch[0] - '0';
            if(scratch[1] != ',') {
                if(scratch[1] < '0' || scratch[1] > '9') break;
                numread *= 10;
                numread += scratch[1] - '0';
                modem->rawRead(1, scratch);
                if(scratch[0] != ',') break;
            }

            modem->rawRead(1, scratch);
            if(scratch[0] != '"') break;
            modem->rawRead(numread, pbuffer);
            modem->rawRead(1, scratch);
            if(scratch[0] != '"') break;

            pbuffer += numread;
            size -= numread;
            totalread += numread;
            if(numread < toread) break;
        } else {
            break;
        }
    }
    return totalread;
}

int UBlox::readFileLine(const char* filename, int offset, char* buffer, int size) {
    int numread = readFile(filename, offset, buffer, size);
    int index = -1;
    for(int i=0; i<numread; i++) {
        if(buffer[i] == '\r') {
            index = i;
            break;
        }
    }

    if(index != -1 && buffer[index+1]=='\n') {
        buffer[index] = 0;
        debug("found line: ");
        debugln(buffer);
        return index+offset+2;
    }
    debugln("line not found");
    return -1;
}

const char* UBlox::query(const char* cmd, uint32_t timeout, uint32_t retries) {
    if(isReady() && MODEM_OK == modem->query(cmd, timeout, retries)) {
        return modem->lastResponse();
    }
    return NULL;
}

const char* UBlox::command(const char* cmd, uint32_t timeout, uint32_t retries) {
    if(isReady() && MODEM_OK == modem->command(cmd, timeout, retries)) {
        return modem->lastResponse();
    }
    return NULL;
}

const char* UBlox::set(const char* cmd, const char* value, uint32_t timeout, uint32_t retries) {
    if(isReady() && MODEM_OK == modem->set(cmd, value, timeout, retries)) {
        return modem->lastResponse();
    }
    return NULL;
}
