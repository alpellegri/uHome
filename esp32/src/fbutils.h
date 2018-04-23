#ifndef FBUTILS_H
#define FBUTILS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <string>
#include <vector>

#define NUM_IO_ENTRY_MAX 20
#define NUM_IO_FUNCTION_MAX 20

enum {
  /*  0 */ kPhyIn = 0,
  /*  1 */ kPhyOut,
  /*  2 */ kDhtTemperature,
  /*  3 */ kDhtHumidity,
  /*  4 */ kRadioRx,
  /*  5 */ kRadioMach,
  /*  6 */ kRadioTx,
  /*  7 */ kTimer,
  /*  8 */ kBool,
  /*  9 */ kInt,
  /* 10 */ kFloat,
  /* 11 */ kMessaging,
};

// template class std::basic_string<char>;

class IoEntry {
public:
  uint8_t code;
  bool ev;
  bool wb;
  String key;
  String value;
  String cb;
  uint32_t ev_value;
};

class FuncEntry {
public:
  uint8_t code;
  String value;
};

class ProgEntry {
public:
  String key; // firebase key
  std::vector<FuncEntry> funcvec;
};

extern std::vector<IoEntry> IoEntryVec;
extern std::vector<ProgEntry> ProgVec;

extern void FB_deinitIoEntryDB(void);
extern void FB_deinitProgDB(void);
extern IoEntry &FB_getIoEntry(uint8_t i);
extern uint8_t FB_getIoEntryLen(void);

extern void FB_addIoEntryDB(String key, JsonObject &obj);
extern String &FB_getIoEntryNameById(uint8_t i);

extern void FB_addProgDB(String key, JsonObject &obj);
extern uint8_t FB_getProgIdx(const char *key);

extern uint8_t FB_checkRadioCodeDB(uint32_t code);
extern uint8_t FB_checkRadioCodeTxDB(uint32_t code);
extern uint8_t FB_getIoEntryIdx(const char *key);
extern uint8_t FB_getFunctionIdx(const char *key);

extern void FB_dumpIoEntry(void);
extern void FB_dumpProg(void);

#endif
