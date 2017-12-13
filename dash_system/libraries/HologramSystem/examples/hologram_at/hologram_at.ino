/*
  hologram_at.ino - Main system firmware

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
#include <HologramSystem.h>

#define HOLO_REV 3

#define MAX_COMMAND_SIZE 256
#define MAX_MESSAGE_SIZE 4*1024
#define MAX_TOPICS 10
#define MAX_TOPIC_SIZE 63
#define MAX_READ_SIZE 64

typedef enum {
  MS_STARTUP,
  MS_RUN,
  MS_PASSTHROUGH,
}modem_state;

modem_state state;

uint8_t read_buffer[MAX_READ_SIZE];

sms_event sms;

uint8_t ipc_cmd_buf[MAX_COMMAND_SIZE];
uint8_t ipc_msg_buf[MAX_MESSAGE_SIZE];
char ipc_topics[MAX_TOPICS][MAX_TOPIC_SIZE+1];
uint32_t ipc_topic_count = 0;
uint32_t ipc_msg_len = 0;
uint32_t ipc_cmd_len = 0;
const char* ipc_topic_list[] =
{&ipc_topics[0][0], &ipc_topics[1][0], &ipc_topics[2][0],
 &ipc_topics[3][0], &ipc_topics[4][0], &ipc_topics[5][0],
 &ipc_topics[6][0], &ipc_topics[7][0], &ipc_topics[7][0],
 &ipc_topics[8][0], &ipc_topics[9][0]};

int getOldestSMS(sms_event &oldest) {
  if(ublox.getNumSMS() == 0) return 0;

  int index_oldest = 0;
  for(int i=1; i<=ublox.getSlotsSMS(); i++) {
    if(ublox.readSMS(i, oldest)) {
      index_oldest = i;
//      Serial.print("+DEBUG: found oldest ");
//      Serial.print(index_oldest);
//      Serial.print(" @ ");
//      Serial.println(Cloud.getSecondsUTC(oldest.timestamp));
      break;
    }
  }
  if(index_oldest == 0) return 0;

  uint32_t numsms = ublox.getNumSMS();
  if(numsms == 1) return index_oldest;

  int i = 1;
  int numcomp = 1;
  static sms_event current;
  uint32_t seconds_oldest = Cloud.getSecondsUTC(oldest.timestamp);

  for(int i=index_oldest+1; i<=ublox.getSlotsSMS() && numcomp < numsms; i++) {
    if(ublox.readSMS(i, current)) {
      numcomp++;
      uint32_t seconds_current = Cloud.getSecondsUTC(current.timestamp);
      if(seconds_current < seconds_oldest) {
        seconds_oldest = seconds_current;
        index_oldest = i;
        memcpy(&oldest, &current, sizeof(sms_event));
      }
    }
    if(numsms != ublox.getNumSMS()) {
      //restart the loop because we got a new SMS
      //and it could be in any memory location
      i=0;
      numcomp = 0;
      numsms = ublox.getNumSMS();
    }
  }
  return index_oldest;

}

void sendNextSMS() {
  rtc_datetime_t sms_dt;

  int index = getOldestSMS(sms);
  if(index > 0) {
    OK();
    if(Cloud.checkOTA(index, sms)) {
      return;
    }

    if(Cloud.convertToLocalTime(sms_dt, sms.timestamp)) {
      ublox.deleteSMS(index);

      Serial.print("+HHSMSCTX: \"+");
      Serial.print(sms.sender);
      Serial.print("\",\"");
      Serial.print(sms_dt);
      Serial.print("\",");
      Serial.println(strlen(sms.message));
      Serial.println(sms.message);
      return;
    }
  }

  ERROR();
}

void handle_event(ublox_event_id id, const ublox_event_content *content) {
  switch(id) {
    case UBLOX_EVENT_SMS_RECEIVED:
      Serial.print("+HHSMSRX: ");
      Serial.println(ublox.getNumSMS());
      break;
    case UBLOX_EVENT_SOCKET_ACCEPT:
      Cloud.acknowledgeAccept(content->accept.socket);
      Serial.print("+HHSOCKACCEPT: ");
      Serial.print(content->accept.socket);
      Serial.print(",\"");
      Serial.print(content->accept.remote.host);
      Serial.print("\",");
      Serial.print(content->accept.remote.port);
      Serial.print(",");
      Serial.println(content->accept.listener);
      Serial.println();
      break;
    case UBLOX_EVENT_CONNECTED:
      Serial.println("+HHCONNECTED: 1");
      break;
    case UBLOX_EVENT_FORCED_DISCONNECT:
      Serial.println("+HHCONNECTED: 0");
      break;
    case UBLOX_EVENT_NETWORK_UNREGISTERED:
      Serial.println("+HHREGISTERED: 0");
      break;
    case UBLOX_EVENT_NETWORK_REGISTERED:
      Serial.println("+HHREGISTERED: 1");
      break;
    case UBLOX_EVENT_NETWORK_TIME_UPDATE:
      break;
    case UBLOX_EVENT_LOCATION_UPDATE:
      //+HHLOC: "2011/04/13,09:54:51",45.6334520,13.0618620,49,1
      //convert location timestamp to local time
      rtc_datetime_t loc_dt;
      Cloud.convertToLocalTime(loc_dt, content->location.timestamp);
      Serial.print("+HHLOC: \"");
      Serial.print(loc_dt);
      Serial.print("\",");
      Serial.print(content->location.lat);
      Serial.print(",");
      Serial.print(content->location.lon);
      Serial.print(",");
      Serial.print(content->location.altitude);
      Serial.print(",");
      Serial.println(content->location.uncertainty);
      break;
    }
}

typedef enum {
  ST_FIND_A,
  ST_FIND_T,
  ST_IN_COMMAND,
  ST_FIND_EQ,
  ST_FIND_SET,
  ST_IN_SET,
  ST_IN_ERROR,
}parse_state;

typedef enum {
  PR_NEXTCHAR,
  PR_COMMAND,
  PR_SET,
  PR_ERROR,
}process_state;

parse_state st = ST_FIND_A;

void ERROR() {
  if(Serial.available() > 1) {
    Serial.flush();
  } else {
  }
  Serial.println("ERROR");
}

void OK() {
  if(Serial.available() > 1) {
    Serial.flush();
    Serial.println("ERROR");
  } else {
    Serial.println("OK");
  }
}

void respond(const char* cmd, int value) {
  Serial.print(cmd);
  Serial.print(": ");
  Serial.println(value);
  Serial.println();
  OK();
}

void processQuery(const char* query) {
  if(strcmp(query, "+HOLO")==0) {
    respond(query, HOLO_REV);
  } else if (strcmp(query, "+HSMS")==0) {
    respond(query, ublox.isNetworkTimeAvailable() ? ublox.getNumSMS() : 0);
  } else if(strcmp(query, "+HCHARGE")==0) {
    respond(query, (int)System.chargeState());
  } else {
    const char* r = ublox.query(query);
    if(r == NULL) {
      ERROR();
    } else {
      if(strlen(r) > 0) {
        Serial.println(r);
        Serial.println();
      }
      OK();
    }
  }
}

void processCommand(const char* cmd) {
  if(strcmp(cmd, "")==0) {
    OK();
  } else if(strcmp(cmd, "+HMRST")==0) {
    ipc_topic_count = 0;
    ipc_msg_len = 0;
    OK();
  } else if(strcmp(cmd, "+HDEBUGTIMEOUT")==0) {

  } else if(strcmp(cmd, "+HSQ")==0) {
    volatile uint8_t *p = 0;
    p[0] = strlen(cmd);
    respond(cmd, p[0]);
  } else if(strcmp(cmd, "+HCONSTATUS")==0) {
    respond(cmd, ublox.getConnectionStatus());
  } else if(strcmp(cmd, "+HCONNECT")==0) {
    ublox.powerUp();
    OK();
  } else if(strcmp(cmd, "+HDISCONNECT")==0) {
    OK();
    ublox.powerDown();
  } else if(strcmp(cmd, "+HSHUTDOWN")==0) {
    ublox.powerDown();
    OK();
    Serial.waitToEmpty();
    Serial.end();
    SerialUBlox.end();
    pinMode(USR_TX_TO_SYS_RX, INPUT_PULLDOWN);
    System.shutdown();
    ublox.powerUp();
    restart();
  } else if(strcmp(cmd, "+HMSEND")==0) {
    if(ipc_msg_len > 0) {
      if(Cloud.sendMessage(ipc_msg_buf, ipc_msg_len, ipc_topic_list, ipc_topic_count)) {
        ipc_msg_buf[ipc_msg_len] = 0;
        OK();
      } else {
        ERROR();
      }
      ipc_msg_len = 0;
      ipc_topic_count = 0;
    } else {
      ERROR();
    }
  } else if(strcmp(cmd, "+HSMSRD")==0) {
    sendNextSMS();
  } else if(strcmp(cmd, "+HMODEMRESET")==0) {
    ublox.powerUp();
    OK();
  } else {
    const char* r = ublox.command(cmd);
    if(r == NULL) {
      ERROR();
    } else {
      if(strlen(r) > 0) {
        Serial.println(r);
        Serial.println();
      }
      OK();
    }
  }
}

void processSet(const char* cmd, const char* set) {
  if(strcmp(cmd, "+HTAG")==0 || strcmp(cmd, "+HTOPIC")==0) {
    if(ipc_topic_count < MAX_TOPIC_SIZE && strlen(set) <= MAX_TOPIC_SIZE) {
      strcpy(ipc_topics[ipc_topic_count++], set);
      OK();
    } else {
      ERROR();
    }
  } else if(strcmp(cmd, "+HMWRITE")==0) {
    //AT+HMWRITE=<len>\r\n
    //@
    //<data>
    int wrlen = 0;
    int wrover = 0;
    if(sscanf(set, "%d", &wrlen) == 1) {
      if(wrlen > 128) {
        ERROR();
      } else {
        if(wrlen + ipc_msg_len > MAX_MESSAGE_SIZE)
        {
          wrover = ipc_msg_len + wrlen - MAX_MESSAGE_SIZE;
          wrlen -= wrover;
        }
        System.snooze(2);
        Serial.flush();
        Serial.write('@');
        int towr = wrlen;
        while(towr) {
          if(Serial.available()) {
            ipc_msg_buf[ipc_msg_len++] = Serial.read();
            towr--;
          }
        }
        while(wrover) {
          if(Serial.available()) {
            Serial.read();
            wrover--;
          }
        }
        respond(cmd, wrlen);
        ipc_msg_buf[ipc_msg_len] = 0;
      }
    } else {
      ERROR();
    }
  } else if(strcmp(cmd, "+HSYS")==0) {
    bool boot = true;
    if(strcmp(set, "1") == 0) {
      boot = true;
    } else if(strcmp(set, "2") == 0) {
      boot = false;
    } else {
      ERROR();
      return;
    }

    Serial.print(cmd);
    Serial.print(": ");
    Serial.print(set);
    Serial.print(",\"");
    if(boot) {
      Serial.print(System.bootVersion());
    } else {
      Serial.print(FIRMWARE_VERSION_STRING);
    }
    Serial.println("\"");
    Serial.println();
    OK();
  } else if(strcmp(cmd, "+HRGBN")==0) {
    if(DASH_1_2 && RGB.on(set)) {
      OK();
    } else {
      ERROR();
    }
  } else if(strcmp(cmd, "+HRGBH")==0) {
    char *pEnd;
    int value = strtol(set, &pEnd, 16);
    if(!DASH_1_2 || pEnd[0] != 0) {
        ERROR();
    } else {
      RGB.on(value);
      OK();
    }
  } else if(strcmp(cmd, "+HLED")==0) {
    if(strcmp("1", set) == 0) {
      System.onLED();
      OK();
    } else if(strcmp("0", set) == 0) {
      System.offLED();
      OK();
    } else {
      ERROR();
    }
  } else if(strcmp(cmd, "+HDEBUGDELAYERR")==0) {
    System.snooze(atoi(set));
    ERROR();
  } else if(strcmp(cmd, "+HDEBUGDELAY")==0) {
    System.snooze(atoi(set));
    OK();
  } else if(strcmp(cmd, "+HLOC")==0) {
    int timeout, accuracy;
    if(sscanf(set, "%d,%d", &timeout, &accuracy) == 2) {
      if(ublox.getLocation(2,2,0,timeout,accuracy)) {
        OK();
      } else {
        ERROR();
      }
    } else {
      ERROR();
    }
  } else if(strcmp(cmd, "+HSOCKLISTEN")==0) {
    int port;
    if(sscanf(set, "%d", &port) == 1) {
      int socket = Cloud.listen(port);
      if(socket == 0) {
        ERROR();
      } else {
        respond(cmd, socket);
      }
    } else {
      ERROR();
    }
  } else if(strcmp(cmd, "+HSOCKREAD")==0) {
    int socket, maxlen, timeout, hex;
    if(sscanf(set, "%d,%d,%d,%d", &socket, &maxlen, &timeout, &hex) == 4) {
      if(hex == 1) {
        if(maxlen > MAX_READ_SIZE/2) maxlen = MAX_READ_SIZE/2;
      } else {
        if(maxlen > MAX_READ_SIZE) maxlen = MAX_READ_SIZE;
      }
      int r = ublox.read(socket, maxlen, read_buffer, timeout, hex == 1);
      if(r == -1) {
        ERROR();
      } else {
        Serial.print("+HSOCKREAD: ");
        Serial.print(socket);
        Serial.print(",");
        Serial.print(hex == 1 ? 1 : 0);
        Serial.print(",");
        Serial.print(r);
        Serial.print(",\"");
        if(hex == 1) r *= 2;
        for(int i=0; i<r; i++)
          Serial.write(read_buffer[i]);
        Serial.print("\"");
        Serial.println();
        Serial.println();
        OK();
      }
    } else {
      ERROR();
    }
  } else if(strcmp(cmd, "+HSOCKCLOSE")==0) {
    int socket;
    if(sscanf(set, "%d", &socket) == 1) {
      ublox.close(socket);
      OK();
    } else {
      ERROR();
    }
  } else if(strcmp(cmd, "+HPASSTHROUGH")==0) {
    int value;
    if(sscanf(set, "%d", &value) == 1) {
      OK();
      state = MS_PASSTHROUGH;
    } else {
      ERROR();
    }
  } else {
    const char* r = ublox.set(cmd, set);
    if(r == NULL) {
      ERROR();
    } else {
      if(strlen(r) > 0) {
        Serial.println(r);
        Serial.println();
      }
      OK();
    }
  }
}

void processError(const char* cmd, const char* set) {
  Serial.println("ERROR");
}

void pushChar(uint8_t c)
{
  process_state pr = PR_NEXTCHAR;
  static char* pset = NULL;
  ipc_cmd_buf[ipc_cmd_len] = c;

  switch(st) {
    case ST_FIND_A:
      if(c == 'A' || c == 'a') {
        st = ST_FIND_T;
      } else if (c == '\r' || c == '\n' || c == ' ') {
        return; //skip
      } else {
        st = ST_IN_ERROR;
      }
      break;
    case ST_FIND_T:
      st = (c == 'T' || c == 't') ? ST_IN_COMMAND : ST_IN_ERROR;
      break;
    case ST_IN_COMMAND:
      if(c == ' ') {
        ipc_cmd_buf[ipc_cmd_len] = 0;
        st = ST_FIND_EQ;
      } else if (c == '=') {
        ipc_cmd_buf[ipc_cmd_len] = 0;
        st = ST_FIND_SET;
      } else if (c == '\r' || c == '\n') {
        pr = PR_COMMAND;
        ipc_cmd_buf[ipc_cmd_len] = 0;
      } else if(c >= 'a' && c <= 'z') {
        ipc_cmd_buf[ipc_cmd_len] -= 32; //capitalize
      }
      break;
    case ST_FIND_EQ:
      if(c == '=') {
        st = ST_FIND_SET;
      } else if (c == '\r' || c == '\n') {
        pr = PR_COMMAND;
      } else if (c == ' ') {
        return; //skip
      } else {
        st = ST_IN_ERROR;
      }
      break;
    case ST_FIND_SET:
      if (c == '\r' || c == '\n') {
        st = ST_IN_ERROR;
      } else if (c == ' ') {
        return; //skip
      } else {
        pset = (char*)&ipc_cmd_buf[ipc_cmd_len];
        st = ST_IN_SET;
      }
      break;
    case ST_IN_SET:
      if (c == '\r' || c == '\n') {
        ipc_cmd_buf[ipc_cmd_len] = 0;

        //trim trailing ' '
        int z=ipc_cmd_len-1;
        while(ipc_cmd_buf[z] == ' ') {
          ipc_cmd_buf[z] = 0;
          z--;
        }

        pr = PR_SET;
      }
      break;
    case ST_IN_ERROR:
      if (c == '\r' || c == '\n') {
        pr = PR_ERROR;
      }
  }

  if(pr == PR_NEXTCHAR) {
    ipc_cmd_len++;
  } else {
    if(pr == PR_COMMAND) {
      char* pcmd = (char*)(ipc_cmd_buf+2);
      int len = strlen(pcmd);
      if(len == 0) {
        OK();
        Serial.println("+HDEBUG: AT");
      } else if(len > 0 && pcmd[len-1] == '?') {
        pcmd[len-1] = 0;
        processQuery(pcmd);
      } else {
        processCommand(pcmd);
      }
    } else if(pr == PR_SET) {
      processSet((const char*)(ipc_cmd_buf+2), pset);
    } else if(pr == PR_ERROR) {
      processError((const char*)(ipc_cmd_buf+2), pset);
    }
    ipc_cmd_len = 0;
    pset = NULL;
    st = ST_FIND_A;
  }
}

void chargeStateChanged(CHARGE_STATE cs) {
  Serial.print("+HHCHARGE: ");
  Serial.println((int)cs);
}

CHARGE_STATE updateCharge() {
  static CHARGE_STATE prev = CHARGE_UNKNOWN;
  CHARGE_STATE curr = System.chargeState();
  if(prev != curr) {
    uint32_t start = millis();
    while (millis() - start < 30) {
      for(int i=0; i<100; i++) {
        if(System.chargeState() != curr) {
          return prev;
        }
      }
    }
    prev = curr;
    Serial.print("+HHCHARGE: ");
    Serial.println((int)prev);
  }
  return prev;
}

volatile bool ublox_low = true;
void ublox_on_check() {
  ublox_low = false;
}

void check_ublox() {
  pinMode(SYS_UBLOX_RX, INPUT_PULLDOWN);
  if(digitalRead(SYS_UBLOX_RX) == LOW) {
    attachInterrupt(SYS_UBLOX_RX, ublox_on_check, RISING);
    uint32_t startMillis = millis();
    while(ublox_low && (millis() - startMillis < 100));
    if(ublox_low && digitalRead(SYS_UBLOX_RX) == LOW) {
        //RGB.on(RED);
        System.ubloxReset();
    }
  }
}

void initialize() {
  SerialUBlox.begin(115200);
  Serial.begin(115200);
  Serial.print("+HHOLO: ");
  Serial.println(HOLO_REV);

  Cloud.begin(ublox, AUTH_TOTP, handle_event);
  ublox.begin(Cloud, SerialUBlox);

  SerialUBlox.flush();
  state = MS_RUN;
}

void state_startup() {
  check_ublox();
  initialize();
}

void state_run() {
  bool sleep = true;
  ublox.pollEvents();
  updateCharge();

  while(Serial.available()) {
    sleep = false;
    pushChar(Serial.read());
  }

  if(sleep) {
    Serial.waitToEmpty();
    System.sleep();
  }
}

void setup() {
  updateCharge();
  state = MS_STARTUP;
  ublox_low = true;
}

void restart() {
  wvariant_init();
  initialize();
}

void loop() {
  switch(state) {
    case MS_STARTUP:
      state_startup();
      break;
    case MS_RUN:
      state_run();
      break;
    case MS_PASSTHROUGH:
      while(Serial.available()) {
        SerialUBlox.write(Serial.read());
      }
      while(SerialUBlox.available()) {
        Serial.write(SerialUBlox.read());
      }
      break;
    default:
      break;
  }
}
