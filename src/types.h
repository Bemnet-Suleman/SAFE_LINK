#pragma once
#include <stdint.h>
enum Activity : uint8_t { ACT_UNKNOWN = 0, ACT_STILL = 1, ACT_MOVING = 2 };
// event flag bits sent in "f"
enum : uint8_t { FLAG_SOS = 1, FLAG_FALL = 2, FLAG_INACTIVITY = 4, FLAG_STRAP_OPEN = 8, FLAG_LOW_BATT = 16, FLAG_CRIT_BATT = 32, FLAG_BOOT = 64, FLAG_LATE = 128 };
struct GpsFix { bool valid; int32_t lat_e6; int32_t lon_e6; uint8_t sats; uint16_t hdop_x10; uint32_t sec; };
struct Telemetry {
  uint8_t flags; GpsFix fix; uint16_t mv; uint8_t pct; uint8_t hr; uint8_t hq; uint8_t act;
  bool strapClosed; bool usb; int8_t rssi; uint32_t upSec;
};
