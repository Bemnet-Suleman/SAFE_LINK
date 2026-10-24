#pragma once
#include <stdint.h>
bool imuInit();                 // MPU6050 at 0x68; leaves it in low-power impact-detect mode
bool imuOk();
uint8_t imuClassify();          // Activity: ACT_MOVING / ACT_STILL (about 0.2 s awake)
bool imuFallCheck();            // run after an impact interrupt; blocking up to about 15 s
void imuClearInt();
