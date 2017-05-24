/*
  hologram_uart.ino - system passthrough firmware

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
#include "firmware_version.h"

TOTPAuthentication totp;

void handle_event(ublox_event_id id, ublox_event_content *content) {
  switch(id) {
    case UBLOX_EVENT_SMS_RECEIVED:
      Serial.print("+EVENT:SMSRCVD,");
      Serial.print(strlen(content->sms.message));
      Serial.print(",");
      Serial.println(content->sms.message);

      Serial.print("SMS received from ");
      Serial.print(content->sms.sender);
      Serial.print(" at ");
      Serial.print(content->sms.timestamp);
      Serial.println(":");
      Serial.println(content->sms.message);
      break;
    case UBLOX_EVENT_FORCED_DISCONNECT:
      Serial.println("Forced disconnect");
    }
}

void setup() {
  SerialUBlox.begin(115200);
  Serial.begin(115200);
  Serial.println("System Firmware DASH_UART_" FIRMWARE_VERSION_STRING);
  modem.begin(SerialUBlox, ublox);
  ublox.begin(cloud, modem);
  cloud.begin(ublox, totp, handle_event);
}

char msgbuf[4*1024];
char* msgp = msgbuf;
uint32_t retry_count = 0;

void loop() {
  ublox.pollEvents();
  while(!ublox.getConnectionStatus()) {
    if(retry_count) {
      Serial.print("Waiting ");
      Serial.print(retry_count);
      Serial.println(" minutes to reconnect");
      ublox.powerDown();
      Serial.waitToEmpty();
      System.deepestSleepSec(retry_count*60);
      ublox.powerUp();
      if(retry_count > 5)
        retry_count = 5;
    }
    Serial.println("Connecting...");
    if(ublox.connect()) {
      retry_count = 0;
      Serial.println("Modem is now connected");
    } else {
      Serial.print("Failed to connect: ");
      switch(ublox.getConnectionError()) {
        case UBLOX_ERR_NONE: Serial.println("No error."); break;
        case UBLOX_ERR_SIM: Serial.println("Check SIM card."); break;
        case UBLOX_ERR_SIGNAL: Serial.println("No signal. Check antenna."); break;
        case UBLOX_ERR_CONNECT: Serial.println("No connection. Check if SIM is active."); break;
        default: Serial.print("Unknown "); Serial.println(ublox.getConnectionError()); break;
      }
      retry_count++;
    }
  }
  while(Serial.available()) {
    char c = Serial.read();
    //Serial.write(c); //echo
    *msgp = c;

    if(*msgp == '\r' || *msgp == '\n') {
      while(msgp >= msgbuf && (*msgp == '\r' || *msgp == '\n')) {
        *msgp-- = 0;
      }

      if(strlen(msgbuf) > 0) {
        modem.checkURC();
        if(cloud.sendMessage(msgbuf, "DASH_UART_" FIRMWARE_VERSION_STRING)) {
          Serial.println("Message Sent");
        } else {
          Serial.println("Message Send FAILED!");
        }
        msgp = msgbuf;
      }
    } else {
      msgp++;
    }
  }
  ublox.pollEvents();
  System.sleep();
}
