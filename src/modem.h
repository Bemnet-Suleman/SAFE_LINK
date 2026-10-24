#pragma once
#include <stdint.h>
void modemBegin();
bool modemInit();                 // wake, SIM check, wait for network registration (up to about 90 s), enable auto-sleep
void modemReset();                // pulse the RST pin
int8_t modemRssi();               // 0..31, or -1 when unknown
bool modemHttpPost(const char* body, uint16_t len, uint16_t* status);   // true when the HTTP exchange completed; check *status == 200
void modemPowerDown();            // AT+CPOWD=1 (used on critical battery)
