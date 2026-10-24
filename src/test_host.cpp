// Host-side tests for the pure logic (no hardware): SHA-256/HMAC, NMEA parser, payload signing, EEPROM queue.
// Build:  g++ -std=gnu++11 -Itest/stubs -Isrc test/host/test_host.cpp src/sha256.cpp src/gps.cpp src/report.cpp src/store.cpp -o /tmp/safelink_tests && /tmp/safelink_tests
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "sha256.h"
#include "gps.h"
#include "report.h"
#include "store.h"
#include "config.h"
// ---- minimal fakes for what the units under test call ----
volatile bool g_sosFlag=false; volatile uint32_t g_sec=1234;
void wdtKick(){}
void digitalWrite(uint8_t,uint8_t){} int digitalRead(uint8_t){return 1;} void delay(unsigned long){}
unsigned long millis(){ static unsigned long t=0; return t+=1000; }
SoftwareSerial::SoftwareSerial(uint8_t,uint8_t){} void SoftwareSerial::begin(long){} void SoftwareSerial::end(){} bool SoftwareSerial::listen(){return true;}
int SoftwareSerial::available(){return 0;} int SoftwareSerial::read(){return -1;}
static uint8_t EE[1024];
uint8_t eeprom_read_byte(const uint8_t* p){return EE[(uintptr_t)p];} void eeprom_update_byte(uint8_t* p,uint8_t v){EE[(uintptr_t)p]=v;}
uint32_t eeprom_read_dword(const uint32_t* p){uint32_t v; memcpy(&v,EE+(uintptr_t)p,4); return v;} void eeprom_update_dword(uint32_t* p,uint32_t v){memcpy(EE+(uintptr_t)p,&v,4);}
void eeprom_read_block(void* d,const void* s,size_t n){memcpy(d,EE+(uintptr_t)s,n);} void eeprom_update_block(const void* s,void* d,size_t n){memcpy(EE+(uintptr_t)d,s,n);}
static int fails=0;
#define CHECK(c) do{ if(!(c)){ printf("FAIL line %d: %s\n",__LINE__,#c); fails++; } }while(0)
static std::string hex(const uint8_t* b,int n){ std::string s; char t[3]; for(int i=0;i<n;i++){ snprintf(t,3,"%02x",b[i]); s+=t; } return s; }
static std::string sha(const std::string& m){ Sha256 c; uint8_t o[32]; sha256Init(c); sha256Update(c,(const uint8_t*)m.data(),m.size()); sha256Final(c,o); return hex(o,32); }
static std::string hm(const std::string& k,const std::string& m){ uint8_t o[32]; hmacSha256((const uint8_t*)k.data(),k.size(),(const uint8_t*)m.data(),m.size(),o); return hex(o,32); }
static std::string nmea(const char* body){ uint8_t cs=0; for(const char* p=body;*p;p++) cs^=*p; char b[120]; snprintf(b,sizeof b,"$%s*%02X\r\n",body,cs); return b; }
int main(){
  CHECK(sha("")=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
  CHECK(sha("abc")=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
  CHECK(sha("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")=="248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
  CHECK(sha(std::string(1000000,'a'))=="cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
  CHECK(hm(std::string(20,'\x0b'),"Hi There")=="b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
  CHECK(hm("Jefe","what do ya want for nothing?")=="5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
  CHECK(hm(std::string(20,'\xaa'),std::string(50,'\xdd'))=="773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe");
  // NMEA
  GpsFix f; std::string s=nmea("GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,");
  CHECK(nmeaParse(s.c_str(),f)); CHECK(f.valid); CHECK(f.lat_e6==48117300); CHECK(f.lon_e6==11516666); CHECK(f.sats==8); CHECK(f.hdop_x10==9);
  s=nmea("GNGGA,100000.00,0902.1420,N,03845.9060,E,1,07,1.2,2355.0,M,-6.0,M,,");     // Addis Ababa
  CHECK(nmeaParse(s.c_str(),f)&&f.valid); CHECK(f.lat_e6==9035700); CHECK(f.lon_e6==38765100); CHECK(f.hdop_x10==12);
  s=nmea("GPGGA,100000,3351.5000,S,15112.0000,W,2,10,0.8,10.0,M,0.0,M,,");
  CHECK(nmeaParse(s.c_str(),f)&&f.valid); CHECK(f.lat_e6==-33858333); CHECK(f.lon_e6==-151200000);
  s=nmea("GPGGA,100000,,,,,0,00,99.9,,,,,,"); CHECK(nmeaParse(s.c_str(),f)); CHECK(!f.valid);
  s=nmea("GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,"); s[20]^=1; CHECK(!nmeaParse(s.c_str(),f));   // corrupted
  s=nmea("GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W"); CHECK(!nmeaParse(s.c_str(),f));          // not GGA
  // payload: signature over everything before ,"sig":"
  Telemetry t; memset(&t,0,sizeof t); t.flags=FLAG_SOS; t.fix={true,9035700,38765100,9,12,1000}; t.mv=3987; t.pct=71; t.hr=78; t.hq=2; t.act=2; t.strapClosed=true; t.usb=false; t.rssi=17; t.upSec=1234;
  char out[256]; uint16_t n=buildPayload(out,sizeof out,t,42); CHECK(n>0 && n<sizeof out); CHECK(strlen(out)==n);
  printf("%s\n(len %u)\n",out,n);
  const char* sig=strstr(out,",\"sig\":\""); CHECK(sig);
  uint8_t key[32]=HMAC_KEY_BYTES,mac[32]; hmacSha256(key,32,(const uint8_t*)out,sig-out,mac);
  CHECK(std::string(sig+8,32)==hex(mac,16)); CHECK(out[n-1]=='}' && out[n-2]=='"');
  Telemetry big=t; big.fix.lat_e6=-2147483647; big.fix.lon_e6=-2147483647; big.fix.sec=0; big.upSec=4000000000UL; big.mv=65535; big.rssi=-1;
  n=buildPayload(out,sizeof out,big,4000000000UL); CHECK(n>0 && n<sizeof out); printf("worst-case length %u of %u\n",n,(unsigned)sizeof out);
  char tiny[64]; CHECK(buildPayload(tiny,sizeof tiny,t,1)==0);
  // EEPROM store
  memset(EE,0xFF,sizeof EE); storeInit(); uint32_t a=storeNextSeq(),b=storeNextSeq(); CHECK(b==a+1);
  storeInit(); uint32_t c=storeNextSeq(); CHECK(c>b);                      // after a "reset" numbers never go backwards
  EvRec r={}; CHECK(storeCount()==0); for(int i=0;i<25;i++){ r.sec=i; r.flags=1; storePush(r); }
  CHECK(storeCount()==20); EvRec p; CHECK(storePeek(p)); CHECK(p.sec==5);  // oldest 5 were overwritten
  for(int i=0;i<20;i++){ storePeek(p); CHECK(p.sec==(uint32_t)(5+i)); storePop(); } CHECK(storeCount()==0 && !storePeek(p));
  CHECK(sizeof(EvRec)==20);
  printf(fails?"%d FAILURES\n":"ALL TESTS PASSED\n",fails); return fails?1:0;
}
