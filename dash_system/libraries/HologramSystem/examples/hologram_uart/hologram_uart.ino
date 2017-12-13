/*
  hologram_uart.ino - Simple system firmware that sends each line received
  over UART as a Hologram Cloud message. Reports status and errors, along with
  recevied SMS messages.

  NOTE: THIS FILE IS FOR DEMONSTRATION PURPOSES ONLY!
  Use of custom System Firmware, such as this, is not offically supported.
  For best results, use officially provided Hologram Dash System Firmware.

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

sms_event sms;
rtc_datetime_t sms_dt;

void handle_event(ublox_event_id id, const ublox_event_content *content) {
  switch(id) {
    case UBLOX_EVENT_SMS_RECEIVED:
      if(ublox.readSMS(content->sms_index, sms)) {
        Cloud.convertToLocalTime(sms_dt, sms.timestamp);
        ublox.deleteSMS(content->sms_index);
        Serial.print("+EVENT:SMSRCVD,\"");
        Serial.print(sms.sender);
        Serial.print("\",\"");
        Serial.print(sms_dt);
        Serial.print("\",");
        Serial.print(strlen(sms.message));
        Serial.print(",\"");
        Serial.print(sms.message);
        Serial.println("\"");
      }
      break;
    case UBLOX_EVENT_CONNECTED:
      Serial.println("+EVENT:CONNECTED,\"Modem is now connected\"");
      break;
    case UBLOX_EVENT_FORCED_DISCONNECT:
      Serial.println("+EVENT:DISCONNECTED");
      break;
    case UBLOX_EVENT_NETWORK_UNREGISTERED:
      Serial.println("+EVENT:UNREGISTERED");
      break;
    case UBLOX_EVENT_NETWORK_REGISTERED:
      Serial.println("+EVENT:REGISTERED");
      break;
    }
}

int last_status = UBLOX_CONN_ERR_OFF;

void setup() {
  SerialUBlox.begin(115200);
  Serial.begin(115200);
  Serial.println("+EVENT:BOOT");

  Cloud.begin(ublox, AUTH_TOTP, handle_event);
  ublox.begin(Cloud, SerialUBlox);

  Serial.flush();
  Serial.println("+EVENT:CONNECTING,\"Connecting...\"");
  ublox.powerUp();
}

char msgbuf[4*1024];
char* msgp = msgbuf;
uint32_t retry_count = 0;

void loop() {
  ublox.pollEvents();
  int status = ublox.getConnectionStatus();
  if(status != last_status) {

    switch(status) {
        case UBLOX_CONN_ERR_SIM: Serial.println("+EVENT:CONNECTFAIL,\"Failed to connect: Check SIM card.\""); break;
        case UBLOX_CONN_ERR_SIGNAL: Serial.println("+EVENT:CONNECTFAIL,\"Failed to connect: No signal. Check antenna.\""); break;
        case UBLOX_CONN_ERR_CONNECT: Serial.println("+EVENT:CONNECTFAIL,\"Failed to connect: No connection. Check if SIM is active.\""); break;
    }
    last_status = status;  
  }
  while(status == UBLOX_CONN_CONNECTED && Serial.available()) {
    char c = Serial.read();
    //Serial.write(c); //echo
    *msgp = c;

    if(*msgp == '\r' || *msgp == '\n') {
      while(msgp >= msgbuf && (*msgp == '\r' || *msgp == '\n')) {
        *msgp-- = 0;
      }

      if(strlen(msgbuf) > 0) {
        ublox.pollEvents();
        if(Cloud.sendMessage(msgbuf)) {
          Serial.println("+EVENT:SENT,\"Message Sent\"");
        } else {
          Serial.println("+EVENT:SENDFAIL,\"Message Send FAILED!\"");
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
