#include "Secret.h"

#define ceilByteLen(bitLen) ((bitLen) / 8) + ((((bitLen) % 8) > 0) ? 1 : 0)

Secret::Secret(uint16_t secretBitLen): secretBitLen(secretBitLen) {
  int byteLen = ceilByteLen(secretBitLen);
  if (secretBitLen > 0) {
    secret = (uint8_t*) malloc((size_t) byteLen);
  } else {
    secret = nullptr;
  }
}

Secret::Secret(const Secret &other): secretBitLen(other.secretBitLen), name(other.name) {
  if (other.secretBitLen > 0) {
    secret = (uint8_t*) malloc((size_t) ceilByteLen(other.secretBitLen));
    memcpy(secret, other.secret, ceilByteLen(other.secretBitLen));
  } else {
    secret = nullptr;
  }
}

Secret::~Secret() {
  if (secretBitLen > 0) {
    memset(secret, 0, byteLen());
    free(secret);
  }
}

uint8_t* Secret::get() {
  return secret;
}

const uint16_t Secret::bitLen() {
  return secretBitLen;
}

const uint8_t Secret::byteLen() {
  return ceilByteLen(secretBitLen);
}

bool Secret::isValid() {
  return secretBitLen > 0;
}

void Secret::setName(String newName) {
  name = newName;
}

const String Secret::getName() {
  return name;
}
