#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "power.h"
#include "hr.h"
#define ADDR 0x57
static bool s_ok=false;
static bool wr(uint8_t r,uint8_t v){ Wire.beginTransmission(ADDR); Wire.write(r); Wire.write(v); return Wire.endTransmission()==0; }
static bool rd(uint8_t r,uint8_t* b,uint8_t n){
  Wire.beginTransmission(ADDR); Wire.write(r); if(Wire.endTransmission(false)!=0) return false;
  if(Wire.requestFrom((uint8_t)ADDR,n)!=n) return false;
  for(uint8_t i=0;i<n;i++) b[i]=Wire.read();
  return true;
}
bool hrOk(){ return s_ok; }
static void shutdown(){ wr(0x09,0x80); }
bool hrInit(){
  Wire.begin(); uint8_t id=0;
  if(!rd(0xFF,&id,1) || id!=0x15){ s_ok=false; return false; }
  wr(0x09,0x40); delay(50); shutdown(); s_ok=true; return true;
}
static void start(){
  wr(0x09,0x00);                         // leave shutdown
  wr(0x04,0); wr(0x05,0); wr(0x06,0);    // clear FIFO pointers
  wr(0x08,0x5F);                         // average 4 samples (25 Hz result), rollover on
  wr(0x09,0x03);                         // SpO2 mode: red + IR
  wr(0x0A,0x27);                         // 4096 nA range, 100 sps, 411 us (18 bit)
  wr(0x0C,0x24); wr(0x0D,0x24);          // about 7 mA per LED
}
// reads available FIFO samples one by one; returns number processed via callback-free pull
static bool nextSample(uint32_t* ir){
  uint8_t p[3]; if(!rd(0x04,&p[0],1)||!rd(0x06,&p[1],1)) return false;
  if(((p[0]-p[1])&0x1F)==0) return false;
  uint8_t b[6]; if(!rd(0x07,b,6)) return false;
  *ir=(((uint32_t)b[3]<<16)|((uint32_t)b[4]<<8)|b[5])&0x3FFFF;
  return true;
}
bool hrOnSkinQuick(){
  if(!s_ok) return false;
  Wire.begin(); start(); delay(450);
  uint32_t ir=0,last=0; uint8_t n=0;
  while(nextSample(&ir) && n<20){ last=ir; n++; }
  shutdown();
  return last>IR_SKIN_MIN;
}
HrResult hrMeasure(uint8_t seconds){
  HrResult r={0,0,false};
  if(!s_ok) return r;
  Wire.begin(); start();
  const uint16_t DT=40;                           // 25 Hz effective sample period (ms)
  uint32_t ir=0; int32_t dc=0,lp=0,amp=0; bool armed=false; uint32_t n=0;
  int32_t lastBeat=-1; uint16_t iv[12]; uint8_t ni=0; uint32_t irSum=0,irCnt=0;
  uint32_t tEnd=millis()+(uint32_t)seconds*1000UL+500;
  while(millis()<tEnd){
    wdtKick();
    uint8_t guard=0;
    while(nextSample(&ir) && guard++<8){
      if(n==0){ dc=ir; lp=ir; }
      lp+=((int32_t)ir-lp)/2;                      // light smoothing
      dc+=((int32_t)lp-dc)/16;                     // slow baseline
      int32_t ac=lp-dc; int32_t a=ac<0?-ac:ac; amp+=(a-amp)/16;
      irSum+=ir; irCnt++;
      int32_t H=amp/2; if(H<15)H=15;
      if(n>50){                                    // let the baseline settle for 2 s
        if(ac<-H) armed=true;
        else if(ac>H && armed){                    // upward zero crossing = one beat, whatever the polarity
          armed=false; int32_t t=(int32_t)n*DT;
          if(lastBeat>=0){
            int32_t d=t-lastBeat;
            if(d>=300 && d<=2000 && ni<12) iv[ni++]=(uint16_t)d;
          }
          lastBeat=t;
        }
      }
      n++;
    }
    delay(20);
  }
  shutdown();
  uint32_t irMean=irCnt?irSum/irCnt:0; r.onSkin = irMean>IR_SKIN_MIN;
  if(!r.onSkin || ni<3) return r;
  // median, then average of intervals within 30% of it
  uint16_t s[12]; for(uint8_t i=0;i<ni;i++) s[i]=iv[i];
  for(uint8_t i=1;i<ni;i++){ uint16_t k=s[i]; int8_t j=i-1; while(j>=0&&s[j]>k){s[j+1]=s[j];j--;} s[j+1]=k; }
  uint16_t med=s[ni/2]; uint32_t sum=0; uint8_t good=0;
  for(uint8_t i=0;i<ni;i++){ int32_t d=(int32_t)iv[i]-med; if(d<0)d=-d; if(d*10<=(int32_t)med*3){sum+=iv[i];good++;} }
  if(good<3) return r;
  uint16_t mean=sum/good; r.bpm=(uint8_t)(60000UL/mean);
  if(r.bpm<35||r.bpm>200){ r.bpm=0; return r; }
  r.quality = (good>=6 && good*10>=ni*7) ? 2 : 1;
  return r;
}
