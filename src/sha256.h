#pragma once
#include <stdint.h>
#include <stddef.h>
struct Sha256 { uint32_t h[8]; uint8_t buf[64]; uint8_t fill; uint32_t lenBits; };
void sha256Init(Sha256& c);
void sha256Update(Sha256& c, const uint8_t* data, size_t len);
void sha256Final(Sha256& c, uint8_t out[32]);
// key length up to 64 bytes
void hmacSha256(const uint8_t* key, uint8_t keyLen, const uint8_t* msg, size_t msgLen, uint8_t out[32]);
