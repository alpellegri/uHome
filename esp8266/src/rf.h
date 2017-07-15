#ifndef RF_H
#define RF_H

#include <Arduino.h>

extern void RF_ResetRadioCodeDB(void);
extern void RF_ResetRadioCodeTxDB(void);
extern void RF_ResetTimerDB(void);
extern void RF_ResetDoutDB(void);
extern void RF_AddRadioCodeDB(String id, String type, String action, String delay, String action_d);
extern void RF_AddRadioCodeTxDB(String string);
extern void RF_AddTimerDB(String action, String hour, String minute);
extern void RF_AddDoutDB(String action);
extern bool RF_CheckRadioCodeDB(uint32_t code);

extern uint32_t RF_GetRadioCode(void);
extern void RF_Enable(void);
extern void RF_Disable(void);

extern void RF_MonitorTimers(void);

extern void RF_Loop(void);
extern bool RF_Task(void);

#endif
