#pragma once
#include <stdint.h>
extern volatile bool g_sosFlag;
extern volatile bool g_motionFlag;
extern volatile uint32_t g_sec;        // seconds since boot, advanced by the watchdog tick (also while asleep)
void powerInit();
void wdtKick();                         // call at least every WDT_STALL_LIMIT_S seconds, or the MCU resets
uint16_t batteryMv();
uint8_t batteryPercent(uint16_t mv);
bool usbPresent();
void armMotionWake(bool enable);        // allow/disallow the IMU interrupt to wake the MCU
void sleepTick();                       // sleep until the next 1 s tick or a wake interrupt
void softReset();
