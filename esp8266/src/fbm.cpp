#include <Arduino.h>
#include <WiFiUDP.h>

#include <DHT.h>
#include <FirebaseArduino.h>

#include <stdio.h>
#include <string.h>

#include "ee.h"
#include "fblog.h"
#include "fbm.h"
#include "fcm.h"
#include "rf.h"
#include "timesrv.h"

#define DHTPIN D6
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

uint8_t boot_sm = 0;
bool status_alarm = false;
bool status_alarm_last = false;
bool control_alarm = false;
bool control_radio_learn = false;
bool control_radio_update = false;
uint32_t control_time;
uint32_t control_time_last;

uint16_t bootcnt = 0;
uint32_t fbm_code_last = 0;
uint32_t fbm_update_last = 0;
uint32_t fbm_time_th_last = 0;
uint32_t fbm_monitor_last = 0;
bool fbm_monitor_run = false;

uint32_t humidity_data;
uint32_t temperature_data;

uint32_t fbm_update_timer_last;

/**
 * The target IP address to send the magic packet to.
 */
IPAddress computer_ip(192, 168, 1, 255);

/**
 * The targets MAC address to send the packet to
 */
byte mac[] = {0xD0, 0x50, 0x99, 0x5E, 0x4B, 0x0E};

void sendWOL(IPAddress addr, byte *mac, size_t size_of_mac) {
  const byte preamble[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  byte i;

  WiFiUDP udp;

  udp.begin(9);
  udp.beginPacket(addr, 8889);
  udp.write(preamble, sizeof preamble);
  for (i = 0; i < 16; i++) {
    udp.write(mac, size_of_mac);
  }

  udp.endPacket();
  yield();
}

bool FbmUpdateRadioCodes(void) {
  bool ret = true;
  yield();

  if (ret == true) {
    Serial.println(F("FbmUpdateRadioCodes Rx"));
    FirebaseObject ref = Firebase.get(F("RadioCodes/Active"));
    if (Firebase.failed() == true) {
      Serial.print(F("get failed: RadioCodes/Active"));
      Serial.println(Firebase.error());
      ret = false;
    } else {
      RF_ResetRadioCodeDB();
      JsonVariant variant = ref.getJsonVariant();
      JsonObject &object = variant.as<JsonObject>();
      for (JsonObject::iterator i = object.begin(); i != object.end(); ++i) {
        yield();
        // Serial.println(i->key);
        JsonObject &nestedObject = i->value;
        String name = nestedObject["name"];
        String type = nestedObject["type"];
        String action = nestedObject["action"];
        String action_d = nestedObject["action_d"];
        String delay = nestedObject["delay"];
        String id = nestedObject["id"];
        Serial.println(id);
        RF_AddRadioCodeDB(id, name, type, action, delay, action_d);
      }
    }
  }

  if (ret == true) {
    Serial.println(F("FbmUpdateRadioCodes Tx"));
    FirebaseObject ref = Firebase.get(F("RadioCodes/ActiveTx"));
    if (Firebase.failed() == true) {
      Serial.print(F("get failed: RadioCodes/ActiveTx"));
      Serial.println(Firebase.error());
      ret = false;
    } else {
      RF_ResetRadioCodeTxDB();
      JsonVariant variant = ref.getJsonVariant();
      JsonObject &object = variant.as<JsonObject>();
      for (JsonObject::iterator i = object.begin(); i != object.end(); ++i) {
        yield();
        // Serial.println(i->key);
        JsonObject &nestedObject = i->value;
        String id = nestedObject["id"];
        Serial.println(id);
        RF_AddRadioCodeTxDB(id);
      }
    }
  }

  if (ret == true) {
    Serial.println(F("FbmUpdateRadioCodes Timers"));
    FirebaseObject ref = Firebase.get(F("Timers"));
    if (Firebase.failed() == true) {
      Serial.print(F("get failed: Timers"));
      Serial.println(Firebase.error());
      ret = false;
    } else {
      RF_ResetTimerDB();
      JsonVariant variant = ref.getJsonVariant();
      JsonObject &object = variant.as<JsonObject>();
      for (JsonObject::iterator i = object.begin(); i != object.end(); ++i) {
        yield();
        // Serial.println(i->key);
        JsonObject &nestedObject = i->value;
        String type = nestedObject["type"];
        String action = nestedObject["action"];
        String hour = nestedObject["hour"];
        String minute = nestedObject["minute"];
        Serial.println(action);
        RF_AddTimerDB(type, action, hour, minute);
      }
    }
  }

  if (ret == true) {
    Serial.println(F("FbmUpdateDIO Dout"));
    FirebaseObject ref = Firebase.get(F("DIO/Dout"));
    if (Firebase.failed() == true) {
      Serial.print(F("get failed: DIO/Dout"));
      Serial.println(Firebase.error());
      ret = false;
    } else {
      RF_ResetDoutDB();
      JsonVariant variant = ref.getJsonVariant();
      JsonObject &object = variant.as<JsonObject>();
      for (JsonObject::iterator i = object.begin(); i != object.end(); ++i) {
        yield();
        // Serial.println(i->key);
        JsonObject &nestedObject = i->value;
        String id = nestedObject["id"];
        Serial.println(id);
        RF_AddDoutDB(id);
      }
    }
  }

  return ret;
}

/* main function task */
bool FbmService(void) {
  bool ret = false;

  switch (boot_sm) {
  // firebase init
  case 0: {
    bool ret = true;
    char *firebase_url = NULL;
    char *firebase_secret = NULL;

    firebase_url = EE_GetFirebaseUrl();
    firebase_secret = EE_GetFirebaseSecret();
    Firebase.begin(firebase_url, firebase_secret);
    yield();
    Firebase.setBool(F("control/reboot"), false);
    if (Firebase.failed()) {
      Serial.print(F("set failed: control/reboot"));
      Serial.println(Firebase.error());
    } else {
      Serial.println(F("firebase: connected!"));
      boot_sm = 1;
    }
    yield();
  } break;

  // firebase control/status init
  case 1: {
    FirebaseObject fbobject = Firebase.get(F("startup"));
    if (Firebase.failed()) {
      Serial.println(F("set failed: status/bootcnt"));
      Serial.print(Firebase.error());
    } else {
      JsonVariant variant = fbobject.getJsonVariant();
      JsonObject &object = variant.as<JsonObject>();
      if (object.success()) {
        bootcnt = object["bootcnt"];
        object["bootcnt"] = ++bootcnt;
        object["time"] = getTime();
        yield();
        Firebase.set(F("startup"), JsonVariant(object));
        if (Firebase.failed()) {
          bootcnt--;
          Serial.println(F("set failed: status/bootcnt"));
          Serial.println(Firebase.error());
        } else {
          boot_sm = 2;
          Serial.println(F("firebase: configured!"));

          String str = String(ESP.getResetReason());
          fblog_log(str, true);
          yield();
        }
      } else {
        Serial.println(F("parseObject() failed"));
      }
    }
  } break;

  // firebase timer/radio init
  case 2: {
    bool res = FbmUpdateRadioCodes();
    if (res == true) {
      Serial.println(F("Node is up!"));
      boot_sm = 3;
      control_time_last = 0; // trick
    }
  } break;

  // firebase monitoring
  case 3: {
    uint32_t time_now = getTime();
    if ((time_now - fbm_update_last) >=
        ((fbm_monitor_run == true) ? (1) : (5))) {
      Serial.print(F("boot_sm "));
      Serial.print(boot_sm);
      Serial.print(F(": heap "));
      Serial.println(ESP.getFreeHeap());
      fbm_update_last = time_now;

      float h = dht.readHumidity();
      float t = dht.readTemperature();
      if (isnan(h) || isnan(t)) {
        Serial.println(F("Failed to read from DHT sensor!"));
      } else {
        humidity_data = 10 * h;
        temperature_data = 10 * t;
      }
      yield();

      control_time = Firebase.getInt(F("control/time"));
      if (Firebase.failed() == true) {
        Serial.print(F("get failed: control/time"));
        Serial.println(Firebase.error());
      } else {
        if (control_time != control_time_last) {
          control_time_last = control_time;
          fbm_monitor_last = time_now;
          fbm_monitor_run = true;
        }
        if (fbm_monitor_run == true) {
          if ((time_now - fbm_monitor_last) > 10) {
            fbm_monitor_run = false;
          }

          FirebaseObject fbobject = Firebase.get(F("control"));
          if (Firebase.failed() == true) {
            Serial.println(F("get failed: control"));
            Serial.print(Firebase.error());
          } else {
            JsonVariant variant = fbobject.getJsonVariant();
            JsonObject &object = variant.as<JsonObject>();
            if (object.success()) {
              control_alarm = object["alarm"];
              control_radio_learn = object["radio_learn"];
              control_radio_update = object["radio_update"];
              control_time = object["time"];

              bool control_wol = object["wol"];
              if (control_wol == true) {
                Serial.println(F("Sending WOL Packet..."));
                sendWOL(computer_ip, mac, sizeof mac);
              }

              bool control_reboot = object["reboot"];
              if (control_reboot == true) {
                ESP.restart();
              }
            } else {
              Serial.println(F("parseObject() failed"));
            }
          }

          Serial.printf("control_monitor %d, %d\n", time_now, fbm_monitor_last);

          DynamicJsonBuffer jsonBuffer;
          JsonObject &status = jsonBuffer.createObject();
          status["alarm"] = status_alarm;
          status["monitor"] = fbm_monitor_run;
          status["heap"] = ESP.getFreeHeap();
          status["humidity"] = humidity_data;
          status["temperature"] = temperature_data;
          status["time"] = time_now;
          yield();
          Firebase.set(F("status"), JsonVariant(status));
          if (Firebase.failed()) {
            Serial.print(F("set failed: status"));
            Serial.println(Firebase.error());
          }
        }
      }
      yield();

      // log every 30 minutes
      if ((time_now - fbm_time_th_last) > (30 * 60)) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject &th = jsonBuffer.createObject();
        th["time"] = time_now;
        th["t"] = temperature_data;
        th["h"] = humidity_data;
        yield();
        Firebase.push(F("logs/TH"), JsonVariant(th));
        if (Firebase.failed()) {
          Serial.print(F("push failed: logs/TH"));
          Serial.println(Firebase.error());
        } else {
          // update in case of success
          fbm_time_th_last = time_now;
        }
      } else {
        /* do nothing */
      }
    }

    // monitor for alarm activation
    if (status_alarm != control_alarm) {
      status_alarm = control_alarm;
      if (status_alarm == true) {
      }
    }

    // manage RF activation/deactivation
    if ((status_alarm == true) || (control_radio_learn == true)) {
      RF_Enable();
    } else {
      RF_Disable();
    }
    yield();

    // manage alarm activation/deactivation notifications
    if (status_alarm_last != status_alarm) {
      status_alarm_last = status_alarm;
      String str = F("Alarm ");
      str += String((status_alarm == true) ? ("active") : ("inactive"));
      fblog_log(str, false);
      yield();
    }

    if (control_radio_update == true) {
      // clear request
      Firebase.setBool(F("control/radio_update"), false);
      if (Firebase.failed()) {
        Serial.print(F("set failed: control/radio_update"));
        Serial.println(Firebase.error());
      } else {
        // force update DB
        control_radio_update = false;
        boot_sm = 2;
      }
    }
    yield();

    // monitor timers, every 15 sec
    if ((time_now - fbm_update_timer_last) >= 15) {
      fbm_update_timer_last = time_now;
      RF_MonitorTimers();
    }
    yield();

    // monitor for RF radio codes
    uint32_t code = RF_GetRadioCode();
    if (code != 0) {
      uint32_t idx = RF_CheckRadioCodeDB(code);
      if (idx != 0xFF) {
        fbm_monitor_last = time_now;
        fbm_monitor_run = true;
        char hex[10];
        sprintf(hex, "%x", code);
        String str = String(F("Intrusion in: ")) +
                     String(RF_GetRadioName(idx)) + String(F(" !!!"));
        fblog_log(str, status_alarm);
        yield();
      }

      if (control_radio_learn == true) {
        // acquire Active Radio Codes from FB
        if (idx == 0xFF) {
          fbm_monitor_last = time_now;
          fbm_monitor_run = true;
          uint32_t idxTx = RF_CheckRadioCodeTxDB(code);
          if (idxTx == 0xFF) {
            Serial.print(F("RadioCodes/Inactive: "));
            Serial.println(code);
            yield();
            Firebase.setInt(F("RadioCodes/Inactive/last"), code);
            if (Firebase.failed()) {
              Serial.print(F("set failed: RadioCodes/Inactive"));
              Serial.println(Firebase.error());
            } else {
              fbm_code_last = code;
            }
          }
        }
      }
    }
  } break;
  default:
    break;
  }

  return ret;
}
