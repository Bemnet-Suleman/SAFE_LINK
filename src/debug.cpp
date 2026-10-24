#include "debug.h"
#if defined(SAFELINK_DEBUG) && SAFELINK_DEBUG
#include <SoftwareSerial.h>
#include "pins.h"
static SoftwareSerial d(255,PIN_DBG_TX);
void dbgBegin(){ d.begin(9600); }
void dbgPrint(const __FlashStringHelper* s){ d.print(s); }
void dbgPrintln(const __FlashStringHelper* s){ d.println(s); }
void dbgNum(long v){ d.print(v); }
#endif
