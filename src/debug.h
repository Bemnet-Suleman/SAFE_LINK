#pragma once
#include <Arduino.h>
// Debug text on test pad TP1 (PC3), 9600 baud, only when built with -DSAFELINK_DEBUG=1
#if defined(SAFELINK_DEBUG) && SAFELINK_DEBUG
void dbgBegin();
void dbgPrint(const __FlashStringHelper* s);
void dbgPrintln(const __FlashStringHelper* s);
void dbgNum(long v);
#define DBG(s)  dbgPrintln(F(s))
#define DBGN(s,v) do{ dbgPrint(F(s)); dbgNum((long)(v)); dbgPrintln(F("")); }while(0)
#else
inline void dbgBegin(){}
#define DBG(s)  do{}while(0)
#define DBGN(s,v) do{}while(0)
#endif
