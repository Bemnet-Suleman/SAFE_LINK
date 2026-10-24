#include <stdint.h>
#include <avr/eeprom.h>
#include "store.h"
// EEPROM map: [0] magic, [1..4] sequence counter, [5] head, [6] count, [16..] ring of 20 records (20 bytes each)
#define MAGIC 0x5A
#define OFF_SEQ 1
#define OFF_HEAD 5
#define OFF_COUNT 6
#define OFF_RING 16
#define REC_SZ 20
#define CAP 20
static uint32_t s_seq=0;
void storeInit(){
  if(eeprom_read_byte((uint8_t*)0)!=MAGIC){
    eeprom_update_byte((uint8_t*)0,MAGIC); eeprom_update_dword((uint32_t*)OFF_SEQ,0);
    eeprom_update_byte((uint8_t*)OFF_HEAD,0); eeprom_update_byte((uint8_t*)OFF_COUNT,0);
  }
  s_seq=eeprom_read_dword((uint32_t*)OFF_SEQ)+16;            // skip ahead: values handed out since the last save are never reused
  eeprom_update_dword((uint32_t*)OFF_SEQ,s_seq);
  uint8_t c=eeprom_read_byte((uint8_t*)OFF_COUNT),h=eeprom_read_byte((uint8_t*)OFF_HEAD);
  if(c>CAP||h>=CAP){ eeprom_update_byte((uint8_t*)OFF_COUNT,0); eeprom_update_byte((uint8_t*)OFF_HEAD,0); }
}
uint32_t storeNextSeq(){
  s_seq++;
  if((s_seq&7)==0) eeprom_update_dword((uint32_t*)OFF_SEQ,s_seq);
  return s_seq;
}
uint8_t storeCount(){ uint8_t c=eeprom_read_byte((uint8_t*)OFF_COUNT); return c>CAP?0:c; }
bool storePush(const EvRec& r){
  uint8_t c=storeCount(),h=eeprom_read_byte((uint8_t*)OFF_HEAD);
  uint8_t slot=(h+c)%CAP;
  if(c>=CAP){ h=(h+1)%CAP; c=CAP-1; slot=(h+c)%CAP; eeprom_update_byte((uint8_t*)OFF_HEAD,h); }   // overwrite the oldest
  eeprom_update_block(&r,(void*)(uintptr_t)(OFF_RING+slot*REC_SZ),sizeof(EvRec));
  eeprom_update_byte((uint8_t*)OFF_COUNT,c+1);
  return true;
}
bool storePeek(EvRec& r){
  if(storeCount()==0) return false;
  uint8_t h=eeprom_read_byte((uint8_t*)OFF_HEAD);
  eeprom_read_block(&r,(void*)(uintptr_t)(OFF_RING+h*REC_SZ),sizeof(EvRec)); return true;
}
void storePop(){
  uint8_t c=storeCount(); if(!c) return;
  uint8_t h=eeprom_read_byte((uint8_t*)OFF_HEAD);
  eeprom_update_byte((uint8_t*)OFF_HEAD,(h+1)%CAP); eeprom_update_byte((uint8_t*)OFF_COUNT,c-1);
}
