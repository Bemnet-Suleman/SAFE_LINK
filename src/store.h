#pragma once
#include <stdint.h>
struct EvRec { uint32_t sec; int32_t lat; int32_t lon; uint16_t mv; uint8_t flags; uint8_t hr; uint8_t act; uint8_t sats; uint8_t fixValid; uint8_t pct; };
void storeInit();
uint32_t storeNextSeq();             // monotonically increasing, survives resets (server rejects replays)
bool storePush(const EvRec& r);      // unsent critical events (SOS, fall) survive a reset or a dead network
bool storePeek(EvRec& r);
void storePop();
uint8_t storeCount();
