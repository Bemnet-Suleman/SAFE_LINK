#pragma once
#include <stdint.h>
#include "types.h"
extern GpsFix g_lastFix;                                   // last valid fix (valid=false until the first one)
bool nmeaParse(const char* sentence, GpsFix& fix);         // GGA sentences only; checks the checksum
bool gpsGetFix(uint16_t timeoutSec, GpsFix& out);          // powers the GPS on, waits for a fix, powers it off
