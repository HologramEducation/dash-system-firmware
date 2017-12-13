/*
  ArduinoCloud.cpp - Class definitions that provide Arduino layer for Hologram
  Cloud on Dash

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
#include "ArduinoCloud.h"
#include "UBloxStream.h"

Updater OTA;

void ArduinoCloud::begin(UBlox &ublox, AuthenticationMethod method, event_callback cb) {
    init(ublox, method);
    this->ublox = &ublox;
    event_cb = cb;
}

const char* ArduinoCloud::getID() {
    if(iccid[0] == 0) {
        ublox->getICCID(iccid);
    }
    return iccid;
}

const char* ArduinoCloud::getKey() {
    if(imsi[0] == 0) {
        ublox->getIMSI(imsi);
    }
    return imsi;
}

const char* ArduinoCloud::getHost() {
    return "cloudsocket.hologram.io";
}

int ArduinoCloud::getPort() {
    return 9999;
}

uint32_t ArduinoCloud::getSecondsUTC(const timestamp_tz &ts) {
    rtc_datetime_t dt;
    dt.year = ts.year + 2000;
    dt.month = ts.month;
    dt.day = ts.day;
    dt.hour = ts.hour;
    dt.minute = ts.minute;
    dt.second = ts.second;

    uint32_t seconds = 0;
    RTC_HAL_ConvertDatetimeToSecs(&dt, &seconds);

    int tz_seconds = ts.tzquarter * 15 * 60;
    if(tz_seconds < 0) {
      seconds += (uint32_t)(abs(tz_seconds));
    } else {
      seconds -= tz_seconds;
    }
    return seconds;
}

bool ArduinoCloud::convertToUTC(rtc_datetime_t &dest, const timestamp_tz &source) {
    uint32_t seconds = getSecondsUTC(source);
    RTC_HAL_ConvertSecsToDatetime(&seconds, &dest);
    return RTC_HAL_IsDatetimeCorrectFormat(&dest);
}

bool ArduinoCloud::convertToLocalTime(rtc_datetime_t &dest, const timestamp_tz &source) {
    timestamp_tz networktime;
    if(!ublox->getNetworkTime(networktime)) return false;
    uint32_t seconds = getSecondsUTC(source);

    int tz_seconds = (networktime.tzquarter) * 15 * 60;
    if(tz_seconds < 0) {
      seconds -= (uint32_t)(abs(tz_seconds));
    } else {
      seconds += tz_seconds;
    }

    RTC_HAL_ConvertSecsToDatetime(&seconds, &dest);
    return RTC_HAL_IsDatetimeCorrectFormat(&dest);
}

uint32_t ArduinoCloud::getSeconds() {
    timestamp_tz ts;
    if(!ublox->getNetworkTime(ts)) return 0;
    return getSecondsUTC(ts);
}

bool ArduinoCloud::downloadOTA(const char* url, const char* destination) {
    char user[30] = "@dev_sim_";
    char pass[20];

    strcat(user, getID());
    auth->generatePassword(getID(), getKey(), getSeconds(), pass);

    return ublox->httpGet(url, 80, destination, user, pass);
}

int ArduinoCloud::findBlankLine(const char* filename, int offset, char* buffer, int size) {
    while(true) {
        offset = ublox->readFileLine(filename, offset, buffer, size);
        if(offset == -1)
            break;
        if(strlen(buffer) == 0)
            break;
    }
    return offset;
}

int ArduinoCloud::findLine(const char* filename, int offset, const char* linestart, char* buffer, int size) {
    while(true) {
        offset = ublox->readFileLine(filename, offset, buffer, size);
        if(offset == -1)
            break;
        if(strncmp(buffer, linestart, strlen(linestart)) == 0)
            break;
    }
    return offset;
}

bool ArduinoCloud::findContent(const char* filename, int* contentLength, int* contentOffset) {
    char header[80];

    int offset = ublox->readFileLine(filename, 0, header, 80);
    if(offset == -1) return false;
    if(strcmp(header, "HTTP/1.1 200 OK") != 0) return false;

    offset = findLine("ota", 0, "Content-Length:", header, 80);
    if(offset == -1) return false;
    if(sscanf(header, "Content-Length: %d", contentLength) != 1) return false;

    offset = findBlankLine("ota", offset, header, 80);
    if(offset == -1) return false;
    *contentOffset = offset;

    return true;
}

bool ArduinoCloud::programOTA(const char* filename) {
    //find start of content and content length
    int contentLength = 0;
    int contentOffset = 0;
    if(findContent("ota", &contentLength, &contentOffset)) {
        //do OTA update
        System.onLED();
        OTA.init(EZPORT);
        UBloxStream ustream(ublox, "ota", contentOffset, contentLength);
        OTA.updateUserApplication(ustream, contentLength);
        EZPORT.end();
        System.offLED();
    }
}

bool ArduinoCloud::checkOTA(int index, sms_event &ota_sms) {
    const char* payload = auth->validateCommand(ota_sms.message, getID(), getKey(), getSeconds());
    if(payload) {
        ublox->deleteSMS(index);
        if(strncmp(payload, "PF:", 3) == 0) { //OTA message
            System.userInReset(true);
            const char* url = payload+3;
            if(downloadOTA(url, "ota")) {
                if(programOTA("ota")) {

                }
            }
            System.userInReset(false);
        }
        return true;
    }
    return false;
}

void ArduinoCloud::onNetworkEvent(uint32_t id, const void* content) {
    ublox_event_id e = (ublox_event_id)id;
    ublox_event_content *c = (ublox_event_content *)content;

    if(e == UBLOX_EVENT_SMS_RECEIVED) {
        if(ublox->readSMS(c->sms_index, sms)) {
            if(checkOTA(c->sms_index, sms)) {
                return;
            }
        }
    }
    if(event_cb)
        event_cb(e, c);
}

void ArduinoCloud::onPowerUp() {
    if(DASH_1_2) digitalWrite(TXL_ON_N, HIGH);
}

void ArduinoCloud::onPowerDown() {
    if(DASH_1_2) digitalWrite(TXL_ON_N, LOW);
}

ArduinoCloud Cloud;
