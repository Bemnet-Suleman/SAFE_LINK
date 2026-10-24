#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "config.h"
#include "sha256.h"
#include "report.h"
static const uint8_t HMAC_KEY[32] PROGMEM = HMAC_KEY_BYTES;
uint16_t buildPayload(char* out,uint16_t cap,const Telemetry& t,uint32_t seq){
  uint32_t fixAge = t.fix.valid ? (t.upSec - t.fix.sec) : 0;
  int n=snprintf_P(out,cap,
    PSTR("{\"id\":\"" DEVICE_ID "\",\"n\":%lu,\"up\":%lu,\"f\":%u,\"fx\":%u,\"la\":%ld,\"lo\":%ld,\"fa\":%lu,\"sa\":%u,"
         "\"mv\":%u,\"bp\":%u,\"hr\":%u,\"hq\":%u,\"ac\":%u,\"sw\":%u,\"us\":%u,\"rs\":%d,\"fw\":\"" FW_VERSION "\""),
    (unsigned long)seq,(unsigned long)t.upSec,(unsigned)t.flags,(unsigned)(t.fix.valid?1:0),
    (long)t.fix.lat_e6,(long)t.fix.lon_e6,(unsigned long)fixAge,(unsigned)t.fix.sats,
    (unsigned)t.mv,(unsigned)t.pct,(unsigned)t.hr,(unsigned)t.hq,(unsigned)t.act,
    (unsigned)(t.strapClosed?1:0),(unsigned)(t.usb?1:0),(int)t.rssi);
  if(n<=0 || n+2+7+32+2>=cap) return 0;
  uint8_t key[32],mac[32]; memcpy_P(key,HMAC_KEY,32);
  hmacSha256(key,32,(const uint8_t*)out,n,mac);
  memset(key,0,sizeof(key));
  static const char hex[]="0123456789abcdef";
  out[n++]=','; out[n++]='"'; out[n++]='s'; out[n++]='i'; out[n++]='g'; out[n++]='"'; out[n++]=':'; out[n++]='"';
  for(uint8_t i=0;i<16;i++){ out[n++]=hex[mac[i]>>4]; out[n++]=hex[mac[i]&15]; }   // first 16 bytes of the HMAC
  out[n++]='"'; out[n++]='}'; out[n]=0;
  return (uint16_t)n;
}
