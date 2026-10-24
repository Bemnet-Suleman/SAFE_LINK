#pragma once
#include <stdint.h>
struct HrResult { uint8_t bpm; uint8_t quality; bool onSkin; };   // quality: 0 none, 1 poor, 2 good
bool hrInit();                       // MAX30102 at 0x57, left in shutdown
bool hrOk();
HrResult hrMeasure(uint8_t seconds); // blocking; LEDs on only during the window
bool hrOnSkinQuick();                // about 0.4 s IR level check
