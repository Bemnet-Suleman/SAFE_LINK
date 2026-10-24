#include <Arduino.h>
#include <SoftwareSerial.h>
#include <string.h>
#include "pins.h"
#include "config.h"
#include "power.h"
#include "gps.h"
GpsFix g_lastFix={false,0,0,0,0,0};
static SoftwareSerial gpsSerial(PIN_GPS_RX,255);           // receive only, 9600 baud
static int8_t hexv(char c){ if(c>='0'&&c<='9')return c-'0'; if(c>='A'&&c<='F')return c-'A'+10; if(c>='a'&&c<='f')return c-'a'+10; return -1; }
// "ddmm.mmmm" or "dddmm.mmmm" -> micro-degrees
static bool coord(const char* s,uint8_t degDigits,int32_t* out){
  if(strlen(s)<(size_t)degDigits+3) return false;
  int32_t deg=0; for(uint8_t i=0;i<degDigits;i++){ if(s[i]<'0'||s[i]>'9')return false; deg=deg*10+(s[i]-'0'); }
  const char* m=s+degDigits; int32_t mi=0; uint8_t i=0;
  for(;m[i]&&m[i]!='.';i++){ if(m[i]<'0'||m[i]>'9')return false; mi=mi*10+(m[i]-'0'); }
  int32_t frac=0; uint8_t fd=0;
  if(m[i]=='.'){ i++; for(;m[i]&&fd<6;i++,fd++){ if(m[i]<'0'||m[i]>'9')return false; frac=frac*10+(m[i]-'0'); } }
  while(fd<6){ frac*=10; fd++; }
  int32_t min_e6=mi*1000000L+frac;                          // minutes in millionths
  *out=deg*1000000L+min_e6/60;
  return true;
}
bool nmeaParse(const char* sentence,GpsFix& fix){
  if(sentence[0]!='$') return false;
  const char* star=strchr(sentence,'*'); if(!star) return false;
  uint8_t cs=0; for(const char* p=sentence+1;p<star;p++) cs^=(uint8_t)*p;
  int8_t h1=hexv(star[1]),h2=hexv(star[2]); if(h1<0||h2<0||cs!=(uint8_t)((h1<<4)|h2)) return false;
  if(strlen(sentence)<8 || strncmp(sentence+3,"GGA",3)!=0) return false;
  char buf[84]; size_t n=star-sentence; if(n>=sizeof(buf)) return false;
  memcpy(buf,sentence,n); buf[n]=0;
  char* f[15]; uint8_t nf=0; f[nf++]=buf;
  for(char* p=buf;*p&&nf<15;p++) if(*p==','){ *p=0; f[nf++]=p+1; }
  if(nf<9) return false;
  // 0 id,1 time,2 lat,3 N/S,4 lon,5 E/W,6 quality,7 sats,8 hdop
  int q=atoi(f[6]); fix.valid=false; fix.sats=(uint8_t)atoi(f[7]);
  if(q<1) return true;                                      // sentence ok, but no fix yet
  int32_t la,lo; if(!coord(f[2],2,&la)||!coord(f[4],3,&lo)) return false;
  if(f[3][0]=='S') la=-la;
  if(f[5][0]=='W') lo=-lo;
  fix.lat_e6=la; fix.lon_e6=lo;
  int32_t hd=0; uint8_t i=0; for(;f[8][i]&&f[8][i]!='.';i++) hd=hd*10+(f[8][i]-'0');
  hd*=10; if(f[8][i]=='.'&&f[8][i+1]>='0'&&f[8][i+1]<='9') hd+=f[8][i+1]-'0';
  fix.hdop_x10=(uint16_t)hd; fix.valid=true; return true;
}
bool gpsGetFix(uint16_t timeoutSec,GpsFix& out){
  char line[84]; uint8_t idx=0; bool ok=false;
  digitalWrite(PIN_GPS_EN,HIGH); delay(300);
  gpsSerial.begin(9600); gpsSerial.listen();
  uint32_t t0=millis();
  while(millis()-t0<(uint32_t)timeoutSec*1000UL && !ok){
    wdtKick();
    if(g_sosFlag||digitalRead(PIN_SOS)==LOW) break;         // SOS has priority over a GPS fix
    while(gpsSerial.available()){
      char c=gpsSerial.read();
      if(c=='$') idx=0;
      if(idx<sizeof(line)-1) line[idx++]=c;
      if(c=='\n'){
        line[idx]=0; idx=0; GpsFix f={false,0,0,0,0,0};
        if(nmeaParse(line,f) && f.valid && f.sats>=GPS_MIN_SATS){ f.sec=g_sec; out=f; ok=true; break; }
      }
    }
  }
  gpsSerial.end(); digitalWrite(PIN_GPS_EN,LOW);
  return ok;
}
