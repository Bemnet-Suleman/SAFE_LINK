#include "sha256.h"
#include <string.h>
#include <avr/pgmspace.h>
static const uint32_t K[64] PROGMEM = {
0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
#define ROR(x,n) (((x)>>(n))|((x)<<(32-(n))))
static void transform(Sha256& c){
  uint32_t w[16];   // rolling 16-word message schedule: only 64 bytes of stack
  for(uint8_t i=0;i<16;i++) w[i]=((uint32_t)c.buf[i*4]<<24)|((uint32_t)c.buf[i*4+1]<<16)|((uint32_t)c.buf[i*4+2]<<8)|c.buf[i*4+3];
  uint32_t a=c.h[0],b=c.h[1],cc=c.h[2],d=c.h[3],e=c.h[4],f=c.h[5],g=c.h[6],h=c.h[7];
  for(uint8_t i=0;i<64;i++){
    uint32_t wi;
    if(i<16) wi=w[i];
    else{
      uint32_t x=w[(i+1)&15], y=w[(i+14)&15];
      wi=w[i&15]+(ROR(x,7)^ROR(x,18)^(x>>3))+w[(i+9)&15]+(ROR(y,17)^ROR(y,19)^(y>>10));
      w[i&15]=wi;
    }
    uint32_t S1=ROR(e,6)^ROR(e,11)^ROR(e,25), ch=(e&f)^((~e)&g), k=pgm_read_dword(&K[i]);
    uint32_t t1=h+S1+ch+k+wi;
    uint32_t S0=ROR(a,2)^ROR(a,13)^ROR(a,22), mj=(a&b)^(a&cc)^(b&cc), t2=S0+mj;
    h=g;g=f;f=e;e=d+t1;d=cc;cc=b;b=a;a=t1+t2;
  }
  c.h[0]+=a;c.h[1]+=b;c.h[2]+=cc;c.h[3]+=d;c.h[4]+=e;c.h[5]+=f;c.h[6]+=g;c.h[7]+=h;
}
void sha256Init(Sha256& c){
  static const uint32_t iv[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
  for(uint8_t i=0;i<8;i++)c.h[i]=iv[i];
  c.fill=0;c.lenBits=0;
}
void sha256Update(Sha256& c,const uint8_t* d,size_t n){
  c.lenBits+= (uint32_t)n*8;
  while(n--){ c.buf[c.fill++]=*d++; if(c.fill==64){transform(c);c.fill=0;} }
}
void sha256Final(Sha256& c,uint8_t out[32]){
  uint32_t bits=c.lenBits;
  c.buf[c.fill++]=0x80;
  if(c.fill>56){ while(c.fill<64)c.buf[c.fill++]=0; transform(c);c.fill=0; }
  while(c.fill<56)c.buf[c.fill++]=0;
  for(uint8_t i=0;i<4;i++)c.buf[56+i]=0;                 // upper 32 bits of the length (messages here are < 512 MB)
  c.buf[60]=bits>>24;c.buf[61]=bits>>16;c.buf[62]=bits>>8;c.buf[63]=bits;
  transform(c);
  for(uint8_t i=0;i<8;i++){out[i*4]=c.h[i]>>24;out[i*4+1]=c.h[i]>>16;out[i*4+2]=c.h[i]>>8;out[i*4+3]=c.h[i];}
}
void hmacSha256(const uint8_t* key,uint8_t keyLen,const uint8_t* msg,size_t msgLen,uint8_t out[32]){
  uint8_t k[64];memset(k,0,64);
  if(keyLen>64){Sha256 t;sha256Init(t);sha256Update(t,key,keyLen);sha256Final(t,k);}
  else memcpy(k,key,keyLen);
  uint8_t pad[64];Sha256 c;uint8_t inner[32];
  for(uint8_t i=0;i<64;i++)pad[i]=k[i]^0x36;
  sha256Init(c);sha256Update(c,pad,64);sha256Update(c,msg,msgLen);sha256Final(c,inner);
  for(uint8_t i=0;i<64;i++)pad[i]=k[i]^0x5c;
  sha256Init(c);sha256Update(c,pad,64);sha256Update(c,inner,32);sha256Final(c,out);
}
