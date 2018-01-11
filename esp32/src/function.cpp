#include <Arduino.h>
#include <Ticker.h>

#include <stdio.h>
#include <string.h>
#include <vector>

#include "fbconf.h"
#include "fbm.h"
#include "fbutils.h"
#include "rf.h"
#include <FirebaseArduino.h>

#define DEBUG_VM(...) Serial.printf(__VA_ARGS__)

extern std::vector<IoEntry> IoEntryVec;
extern std::vector<FunctionEntry> FunctionVec;

typedef struct {
  uint32_t V;
  uint32_t ACC;
  const char *cb;
} vm_context_t;

typedef struct {
  void (*read)(vm_context_t &ctx, const char *value);
  const char *(*exec)(vm_context_t &ctx, const char *key_value);
  void (*write)(vm_context_t &ctx, const char *key_value);
} itlb_t;

void vm_read0(vm_context_t &ctx, const char *value) {
  DEBUG_VM("vm_read0 value=%s\n", value);
}

void vm_readi(vm_context_t &ctx, const char *value) {
  DEBUG_VM("vm_readi value=%s\n", value);
  ctx.V = atoi(value);
}

void vm_read24(vm_context_t &ctx, const char *value) {
  DEBUG_VM("vm_read24 value=%s\n", value);
  uint8_t id = FB_getIoEntryIdx(value);
  uint32_t mask = (1 << 24) - 1;
  ctx.V = IoEntryVec[id].value & mask;
}

void vm_read(vm_context_t &ctx, const char *key_value) {
  DEBUG_VM("vm_read value=%s\n", key_value);
  uint8_t id = FB_getIoEntryIdx(key_value);
  ctx.V = IoEntryVec[id].value;
}

const char *vm_exec_ex0(vm_context_t &ctx, const char *key_value) {
  DEBUG_VM("vm_exec_ex0 value=%s\n", key_value);
  ctx.ACC = 0;
  return ctx.cb;
}

const char *vm_exec_ldi(vm_context_t &ctx, const char *key_value) {
  DEBUG_VM("vm_exec_ldi value=%s\n", key_value);
  ctx.ACC = ctx.V;
  return ctx.cb;
}

const char *vm_exec_st(vm_context_t &ctx, const char *key_value) {
  DEBUG_VM("vm_exec_st value=%s\n", key_value);
  return ctx.cb;
}

const char *vm_exec_lt(vm_context_t &ctx, const char *key_value) {
  DEBUG_VM("vm_exec_lt value=%s\n", key_value);
  ctx.ACC = (ctx.V < ctx.ACC);
  return ctx.cb;
}

const char *vm_exec_gt(vm_context_t &ctx, const char *key_value) {
  DEBUG_VM("vm_exec_lt value=%s\n", key_value);
  ctx.ACC = (ctx.V > ctx.ACC);
  return ctx.cb;
}

const char *vm_exec_eq(vm_context_t &ctx, const char *key_value) {
  DEBUG_VM("vm_exec_eq value=%s\n", key_value);
  ctx.ACC = (ctx.V == ctx.ACC);
  return ctx.cb;
}

const char *vm_exec_bz(vm_context_t &ctx, const char *key_value) {
  DEBUG_VM("vm_exec_bz value=%s\n", key_value);
  if (ctx.ACC == 0) {
    return key_value;
  }
  return ctx.cb;
}

const char *vm_exec_bnz(vm_context_t &ctx, const char *key_value) {
  DEBUG_VM("vm_exec_bnz value=%s\n", key_value);
  if (ctx.ACC != 0) {
    return key_value;
  }
  return ctx.cb;
}

const char *vm_exec_dly(vm_context_t &ctx, const char *key_value) {
  DEBUG_VM("vm_exec_dly value=%s\n", key_value);
  return ctx.cb;
}

void vm_write0(vm_context_t &ctx, const char *key_value) {
  DEBUG_VM("vm_write0 value=%s\n", key_value);
}

void vm_write(vm_context_t &ctx, const char *key_value) {
  DEBUG_VM("vm_write value=%s\n", key_value);
  uint8_t id = FB_getIoEntryIdx(key_value);
  IoEntryVec[id].value = ctx.ACC;
  IoEntryVec[id].wb = true;
}

void vm_write24(vm_context_t &ctx, const char *key_value) {
  DEBUG_VM("vm_write24 value=%s\n", key_value);
  uint8_t id = FB_getIoEntryIdx(key_value);
  uint32_t mask = (1 << 24) - 1;
  uint32_t value = (IoEntryVec[id].value & (~mask)) | (ctx.ACC & mask);
  IoEntryVec[id].value = value;
  IoEntryVec[id].wb = true;
}

itlb_t VM_pipe[] = {
    /*  0: ex0  */ {vm_read0, vm_exec_ex0, vm_write0},
    /*  1: ldi  */ {vm_readi, vm_exec_ldi, vm_write0},
    /*  2: ld24 */ {vm_read24, vm_exec_ldi, vm_write0},
    /*  3: ld   */ {vm_read, vm_exec_ldi, vm_write0},
    /*  4: st24 */ {vm_read0, vm_exec_st, vm_write24},
    /*  5: st   */ {vm_read0, vm_exec_st, vm_write},
    /*  6: lt   */ {vm_read, vm_exec_lt, vm_write0},
    /*  7: gt   */ {vm_read, vm_exec_gt, vm_write0},
    /*  8: eqi  */ {vm_readi, vm_exec_eq, vm_write0},
    /*  9: eq   */ {vm_read, vm_exec_eq, vm_write0},
    /* 10: bz   */ {vm_read0, vm_exec_bz, vm_write0},
    /* 11: bnz  */ {vm_read0, vm_exec_bnz, vm_write0},
    /* 12: dly  */ {vm_read, vm_exec_dly, vm_write0},
};

const char *VM_decode(vm_context_t &ctx, FunctionEntry &stm) {
  uint32_t code = stm.code;
  String &value = stm.value;

  /* decode-read */
  DEBUG_VM("VM_pipe read\n");
  VM_pipe[code].read(ctx, value.c_str());

  /* decode-execute */
  DEBUG_VM("VM_pipe exec\n");
  ctx.cb = stm.cb.c_str();
  const char *key_stm = VM_pipe[code].exec(ctx, value.c_str());

  /* decode-write */
  DEBUG_VM("VM_pipe write\n");
  VM_pipe[code].write(ctx, value.c_str());

  return key_stm;
}

void VM_readIn(void) {
  uint32_t value;

  /* loop over data elements looking for events */
  for (uint8_t i = 0; i < IoEntryVec.size(); i++) {
    switch (IoEntryVec[i].code) {
    case kPhyIn: {
      // DEBUG_VM("VM_readIn-kPhyIn: %s\n", IoEntryVec[i].name.c_str());
      uint8_t pin = IoEntryVec[i].value >> 24;
      pinMode(pin, INPUT);
      uint32_t mask = (1 << 24) - 1;
      value = digitalRead(pin) & mask;
      if ((IoEntryVec[i].value & mask) != value) {
        IoEntryVec[i].value = (IoEntryVec[i].value & (~mask)) | value;
        DEBUG_VM("VM_readIn: %s, %d, %d\n", IoEntryVec[i].name.c_str(), value,
                 IoEntryVec[i].value);
        IoEntryVec[i].ev = true;
      }
    } break;
    case kInt: {
      value = Firebase.getInt(kgraph + "/" + IoEntryVec[i].key + "/value");
      if (Firebase.failed() == true) {
        DEBUG_VM("get failed: kInt");
      } else {
        if ((IoEntryVec[i].value) != value) {
          IoEntryVec[i].value = value;
          IoEntryVec[i].ev = true;
        }
      }
    } break;
    default:
      // DEBUG_VM("VM_readIn: error\n");
      break;
    }
  }
}

uint8_t VM_findEvent(void) {
  uint8_t i = 0;
  uint8_t idx = 0xFF;
  /* loop over data elements looking for events */
  while ((i < IoEntryVec.size()) && (idx == 0xFF)) {
    if (IoEntryVec[i].ev == true) {
      IoEntryVec[i].ev = false;
      idx = i;
    }
    i++;
  }

  return idx;
}

void VM_writeOut(void) {
  /* loop over data elements looking for write-back requests */
  for (uint8_t i = 0; i < IoEntryVec.size(); i++) {
    if (IoEntryVec[i].wb == true) {
      switch (IoEntryVec[i].code) {
      case kPhyOut: {
        uint32_t mask = (1 << 24) - 1;
        uint8_t pin = IoEntryVec[i].value >> 24;
        pinMode(pin, OUTPUT);
        uint32_t value = IoEntryVec[i].value & mask;
        DEBUG_VM("VM_writeOut: kPhyOut %d, %d\n", pin, value);
        digitalWrite(pin, value);
        IoEntryVec[i].wb = false;
      } break;
      case kInt: {
        uint32_t value = IoEntryVec[i].value;
        DEBUG_VM("VM_writeOut: kInt %d\n", value);
        Firebase.setInt(kgraph + "/" + IoEntryVec[i].key + "/value", value);
        if (Firebase.failed() == true) {
          DEBUG_VM("set failed: kInt");
        } else {
          IoEntryVec[i].wb = false;
        }

      } break;
      default:
        // DEBUG_VM("VM_writeOut: error\n");
        break;
      }
    }
  }
}

void VM_run(void) {
  VM_readIn();
  uint8_t id = VM_findEvent();
  if (id != 0xFF) {
    String key_stm = IoEntryVec[id].cb;

    vm_context_t ctx;
    ctx.V = 0;
    ctx.ACC = 0;
    DEBUG_VM("VM_run start >>>>>>>>>>>>\n");
    while (key_stm.length() != 0) {
      uint8_t id_stm = FB_getFunctionIdx(key_stm.c_str());
      FunctionEntry &stm = FunctionVec[id_stm];

      DEBUG_VM("VM_run start name=%s, ACC=%d V=%d\n", stm.name.c_str(), ctx.ACC,
               ctx.V);
      /* decode */
      key_stm = String(VM_decode(ctx, stm));

      DEBUG_VM("VM_run stop name=%s, ACC=%d V=%d\n", stm.name.c_str(), ctx.ACC,
               ctx.V);
    }
    VM_writeOut();
  }
}
