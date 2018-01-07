#include <Arduino.h>
#include <Ticker.h>

#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include "fbutils.h"

static std::vector<IoEntry> IoEntryVec;
static std::vector<FunctionEntry> FunctionVec;

void FB_deinitIoEntryDB(void) {
  IoEntryVec.erase(IoEntryVec.begin(), IoEntryVec.end());
}

void FB_deinitFunctionDB(void) {
  FunctionVec.erase(FunctionVec.begin(), FunctionVec.end());
}

IoEntry &FB_getIoEntry(uint8_t i) { return IoEntryVec[i]; }

uint8_t FB_getIoEntryLen(void) { return IoEntryVec.size(); }

FunctionEntry &FB_getFunction(uint8_t i) { return FunctionVec[i]; }

uint8_t FB_getFunctionLen(void) { return FunctionVec.size(); }

void FB_addIoEntryDB(String key, uint8_t type, String id, String name,
                     String func) {
  if (IoEntryVec.size() < NUM_IO_ENTRY_MAX) {
    IoEntry entry;
    entry.key = key;
    entry.type = type;
    entry.id = atoi(id.c_str());
    entry.name = name;
    entry.func = func;
    IoEntryVec.push_back(entry);
  }
}

String& FB_getIoEntryNameById(uint8_t i) {
  IoEntry& entry = IoEntryVec[i];
  return entry.name;
}

void FB_addFunctionDB(String key, String type, String action, uint32_t delay,
                      String next) {
  if (FunctionVec.size() < NUM_IO_FUNCTION_MAX) {
    FunctionEntry entry;
    entry.key = key;
    entry.type = atoi(type.c_str());
    entry.action = action;
    entry.delay = delay;
    entry.next = next;
    entry.timer_run = 0;
    entry.src_idx = 0xFF;
    entry.timer = 0;
    FunctionVec.push_back(entry);
  }
}

uint8_t FB_getIoEntryIdx(String &key) {
  uint8_t i = 0;
  uint8_t idx = 0xFF;
  uint8_t res;

  while ((i < IoEntryVec.size()) && (idx == 0xFF)) {
    res = strcmp(IoEntryVec[i].key.c_str(), key.c_str());
    if (res == 0) {
      idx = i;
    }
    i++;
  }

  return idx;
}

uint8_t FB_getFunctionIdx(String &key) {
  uint8_t i = 0;
  uint8_t idx = 0xFF;
  uint8_t res;

  while ((i < FunctionVec.size()) && (idx == 0xFF)) {
    res = strcmp(FunctionVec[i].key.c_str(), key.c_str());
    if (res == 0) {
      idx = i;
    }
    i++;
  }
  return idx;
}

void FB_dumpIoEntry(void) {
  Serial.println(F("FB_dumpIoEntry"));
  for (uint8_t i = 0; i < IoEntryVec.size(); ++i) {
    Serial.printf("%d: %06X, %d, %s, %s, %s\n", i, IoEntryVec[i].id,
                  IoEntryVec[i].type, IoEntryVec[i].key.c_str(),
                  IoEntryVec[i].name.c_str(), IoEntryVec[i].func.c_str());
  }
}

void FB_dumpFunctions(void) {
  Serial.println(F("FB_dumpFunctions"));
  for (uint8_t i = 0; i < FunctionVec.size(); ++i) {
    Serial.printf("%d: %s, %s, %s\n", i, FunctionVec[i].key.c_str(),
                  FunctionVec[i].action.c_str(), FunctionVec[i].next.c_str());
  }
}