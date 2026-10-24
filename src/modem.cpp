#include <Arduino.h>
#include <string.h>
#include <stdlib.h>
#include "pins.h"
#include "config.h"
#include "power.h"
#include "modem.h"
static char rb[72];                         // last response, shared to save RAM
static void flushIn(){ while(Serial.available()) Serial.read(); }
// reads until t1 (returns 1) or t2 (returns 2) appears in the response, or the timeout expires (returns 0)
static uint8_t readUntil(uint32_t tmo,const char* t1,const char* t2){
  uint8_t len=0; rb[0]=0; uint32_t t=millis();
  while(millis()-t<tmo){
    wdtKick();
    while(Serial.available()){
      char c=Serial.read();
      if(len<sizeof(rb)-1){ rb[len++]=c; rb[len]=0; }
      else { memmove(rb,rb+1,sizeof(rb)-2); rb[sizeof(rb)-2]=c; rb[sizeof(rb)-1]=0; }
      if(strstr(rb,t1)) return 1;
      if(t2 && strstr(rb,t2)) return 2;
    }
  }
  return 0;
}
static bool at(const __FlashStringHelper* cmd,uint32_t tmo=2000){
  flushIn(); Serial.println(cmd); return readUntil(tmo,"OK","ERROR")==1;
}
static bool wake(){                          // auto-sleep (AT+CSCLK=2) swallows the first bytes: poke until it answers
  for(uint8_t i=0;i<6;i++){ flushIn(); Serial.print("AT\r"); if(readUntil(300,"OK",0)==1) return true; delay(100); }
  return false;
}
void modemBegin(){ Serial.begin(9600); }
void modemReset(){ pinMode(PIN_SIM_RST,OUTPUT); digitalWrite(PIN_SIM_RST,LOW); delay(200); pinMode(PIN_SIM_RST,INPUT); delay(3000); }
bool modemInit(){
  Serial.begin(9600); delay(100);
  if(!wake()){ modemReset(); if(!wake()) return false; }
  at(F("ATE0")); at(F("AT+CMEE=0")); at(F("AT+CFUN=1"),5000);
  uint32_t t=millis(); bool simOk=false;
  while(millis()-t<20000){ wdtKick(); if(at(F("AT+CPIN?")) && strstr(rb,"READY")){ simOk=true; break; } delay(500); }
  if(!simOk) return false;
  t=millis(); bool reg=false;
  while(millis()-t<60000){ wdtKick(); if(at(F("AT+CREG?")) && (strstr(rb,",1")||strstr(rb,",5"))){ reg=true; break; } delay(1000); }
  at(F("AT+CSCLK=2"));
  return reg;
}
int8_t modemRssi(){
  if(!wake()) return -1;
  if(!at(F("AT+CSQ"))) return -1;
  char* p=strstr(rb,"+CSQ:"); if(!p) return -1;
  int v=atoi(p+5); return (v>=0&&v<=31)?(int8_t)v:-1;
}
static bool bearerUp(){
  if(at(F("AT+SAPBR=2,1"),3000) && strstr(rb,"+SAPBR: 1,1")) return true;
  at(F("AT+SAPBR=3,1,\"Contype\",\"GPRS\""));
  flushIn(); Serial.print(F("AT+SAPBR=3,1,\"APN\",\"")); Serial.print(F(APN)); Serial.println('"'); readUntil(2000,"OK","ERROR");
  at(F("AT+SAPBR=1,1"),30000);               // ERROR here usually just means "already open"
  return at(F("AT+SAPBR=2,1"),3000) && strstr(rb,"+SAPBR: 1,1");
}
bool modemHttpPost(const char* body,uint16_t len,uint16_t* status){
  *status=0;
  if(!wake()) return false;
  if(!bearerUp()) return false;
  at(F("AT+HTTPTERM"),1500);
  if(!at(F("AT+HTTPINIT"),3000)) return false;
  bool ok=false;
  do{
    at(F("AT+HTTPPARA=\"CID\",1"),1000);
    flushIn(); Serial.print(F("AT+HTTPPARA=\"URL\",\"")); Serial.print(F(SERVER_URL)); Serial.println('"');
    if(readUntil(2000,"OK","ERROR")!=1) break;
    at(F("AT+HTTPPARA=\"CONTENT\",\"application/json\""),1000);
    flushIn(); Serial.print(F("AT+HTTPDATA=")); Serial.print(len); Serial.println(F(",8000"));
    if(readUntil(3000,"DOWNLOAD","ERROR")!=1) break;
    Serial.write((const uint8_t*)body,len);
    if(readUntil(10000,"OK","ERROR")!=1) break;
    flushIn(); Serial.println(F("AT+HTTPACTION=1"));
    if(readUntil(45000,"+HTTPACTION: 1,",0)!=1) break;
    uint16_t st=0; uint8_t digits=0; uint32_t t=millis();
    while(millis()-t<1500 && digits<4){
      if(Serial.available()){ char c=Serial.read(); if(c>='0'&&c<='9'){ st=st*10+(c-'0'); digits++; } else break; }
    }
    *status=st; ok=true;
  }while(0);
  at(F("AT+HTTPTERM"),1500);
  return ok;
}
void modemPowerDown(){ wake(); at(F("AT+CPOWD=1"),3000); }
