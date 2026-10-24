#pragma once
#include <stdint.h>
#include "types.h"
// Builds the signed JSON report into out. Returns its length, or 0 if it does not fit.
// The signature covers every byte of the JSON before  ,"sig":"  (see docs/FIRMWARE.md).
uint16_t buildPayload(char* out, uint16_t cap, const Telemetry& t, uint32_t seq);
