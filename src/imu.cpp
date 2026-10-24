#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "pins.h"
#include "config.h"
#include "types.h"
#include "power.h"
#include "imu.h"
#define MPU 0x68
static bool s_ok=false;
static float s_ref[3]={0,0,1};          // last stable gravity direction (in g)
static bool wr(uint8_t r,uint8_t v){ Wire.beginTransmission(MPU); Wire.write(r); Wire.write(v); return Wire.endTransmission()==0; }
static bool rd(uint8_t r,uint8_t* b,uint8_t n){
  Wire.beginTransmission(MPU); Wire.write(r); if(Wire.endTransmission(false)!=0) return false;
  if(Wire.requestFrom((uint8_t)MPU,n)!=n) return false;
  for(uint8_t i=0;i<n;i++) b[i]=Wire.read();
  return true;
}
bool imuOk(){ return s_ok; }
static void lowPower(){ wr(0x6C,0xC7); wr(0x6B,0x28); }   // accel-only, 40 Hz wake cycle, gyros standby, temp off
static void active(){ wr(0x6B,0x08); wr(0x6C,0x07); }     // continuous accel, gyros standby
void imuClearInt(){ Wire.begin(); uint8_t s; rd(0x3A,&s,1); }
bool imuInit(){
  Wire.begin(); uint8_t id=0;
  if(!rd(0x75,&id,1) || id!=0x68){ s_ok=false; return false; }
  wr(0x6B,0x80); delay(100); wr(0x6B,0x00); delay(10);
  wr(0x19,0x07); wr(0x1A,0x01);
  wr(0x1C,0x11);                       // +-8 g (4096 LSB/g), 5 Hz high-pass for motion detect
  wr(0x1F,MOT_THR_32MG); wr(0x20,1);   // impact threshold, duration 1 ms
  wr(0x69,0x15);
  wr(0x37,0xB0);                       // INT active LOW, push-pull, latched, cleared on any read
  wr(0x38,0x40);                       // motion interrupt only
  lowPower(); delay(5); imuClearInt();
  s_ok=true; return true;
}
static bool readAcc(int16_t* a){
  uint8_t b[6]; if(!rd(0x3B,b,6)) return false;
  for(uint8_t i=0;i<3;i++) a[i]=(int16_t)((b[i*2]<<8)|b[i*2+1]);
  return true;
}
// n samples spaced ms apart: mean vector (g), mean absolute deviation of |a| (mg)
static bool sample(uint8_t n,uint8_t ms,float* mean,uint16_t* madMg){
  float mag[12]; float sx=0,sy=0,sz=0; uint8_t got=0;
  if(n>12)n=12;
  for(uint8_t i=0;i<n;i++){
    int16_t a[3]; if(readAcc(a)){
      float x=a[0]/4096.0f,y=a[1]/4096.0f,z=a[2]/4096.0f; sx+=x;sy+=y;sz+=z;
      mag[got++]=sqrtf(x*x+y*y+z*z);
    }
    delay(ms);
  }
  if(got<3) return false;
  mean[0]=sx/got;mean[1]=sy/got;mean[2]=sz/got;
  float m=0; for(uint8_t i=0;i<got;i++) m+=mag[i]; m/=got;
  float d=0; for(uint8_t i=0;i<got;i++) d+=fabsf(mag[i]-m); d/=got;
  *madMg=(uint16_t)(d*1000.0f);
  return true;
}
uint8_t imuClassify(){
  if(!s_ok) return ACT_UNKNOWN;
  Wire.begin(); float m[3]; uint16_t mad;
  if(!sample(6,35,m,&mad)) return ACT_UNKNOWN;
  if(mad<60){ s_ref[0]=m[0];s_ref[1]=m[1];s_ref[2]=m[2]; }   // remember a stable gravity direction
  return mad>MOVING_MAD_MG ? ACT_MOVING : ACT_STILL;
}
static float cosAngle(const float* a,const float* b){
  float na=sqrtf(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]), nb=sqrtf(b[0]*b[0]+b[1]*b[1]+b[2]*b[2]);
  if(na<0.2f||nb<0.2f) return 1.0f;
  return (a[0]*b[0]+a[1]*b[1]+a[2]*b[2])/(na*nb);
}
static bool waitMs(uint16_t ms){            // keeps the watchdog happy; returns false if SOS is being pressed
  uint32_t t=millis();
  while(millis()-t<ms){ wdtKick(); if(digitalRead(PIN_SOS)==LOW) return false; delay(10); }
  return true;
}
// Impact interrupt happened (about 2 g spike). A fall = orientation changed by about 60 deg AND then no movement.
bool imuFallCheck(){
  if(!s_ok) return false;
  Wire.begin(); active(); delay(20);
  if(!waitMs(FALL_SETTLE_MS)){ lowPower(); imuClearInt(); return false; }
  float m[3]; uint16_t mad; bool fall=false;
  if(sample(8,25,m,&mad)){
    float c=cosAngle(s_ref,m);
    if(c*100.0f < FALL_ORIENT_COS_X100){
      fall=true;
      for(uint8_t s=0;s<FALL_STILL_S && fall;s++){
        float m2[3]; uint16_t mad2;
        if(sample(4,30,m2,&mad2) && mad2>FALL_STILL_MAD_MG) fall=false;   // person is moving: not a fall
        if(!waitMs(850)){ fall=false; break; }
      }
    }
  }
  lowPower(); delay(5); imuClearInt();
  return fall;
}
