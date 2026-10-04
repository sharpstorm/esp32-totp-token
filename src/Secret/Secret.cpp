#include "Secret.h"

#include "../Util/Base32.h"

#define ceilByteLen(bitLen) ((bitLen) / 8) + ((((bitLen) % 8) > 0) ? 1 : 0)

Secret::Secret(uint16_t secretBitLen) : secretBitLen(secretBitLen) {
  int byteLen = ceilByteLen(secretBitLen);
  if (secretBitLen > 0) {
    secret = (uint8_t*)malloc((size_t)byteLen);
  } else {
    secret = nullptr;
  }
}

Secret::Secret(const Secret& other)
    : secretBitLen(other.secretBitLen), name(other.name) {
  if (other.secretBitLen > 0) {
    secret = (uint8_t*)malloc((size_t)ceilByteLen(other.secretBitLen));
    memcpy(secret, other.secret, ceilByteLen(other.secretBitLen));
  } else {
    secret = nullptr;
  }
}

Secret::~Secret() {
  if (secret != nullptr) {
    memset(secret, 0, byteLen());
    free(secret);
  }
}

uint8_t* Secret::get() { return secret; }

const uint16_t Secret::bitLen() { return secretBitLen; }

const uint8_t Secret::byteLen() { return ceilByteLen(secretBitLen); }

bool Secret::isValid() { return secretBitLen > 0; }

void Secret::setName(String newName) { name = newName; }

const String Secret::getName() { return name; }

Secret Secret::fromBase32(const uint8_t* base32, size_t len) {
  byte* decoded = nullptr;
  int decodedLen = Base32::fromBase32((byte*)base32, (long)len, decoded);
  if (decodedLen <= 0 || decodedLen > 255) {
    if (decoded != nullptr) {
      memset(decoded, 0, decodedLen > 0 ? decodedLen : 0);
      free(decoded);
    }
    return Secret(0);
  }

  Secret secret((uint16_t)(decodedLen * 8));
  if (secret.get() == nullptr) {
    memset(decoded, 0, decodedLen);
    free(decoded);
    return Secret(0);
  }
  memcpy(secret.get(), decoded, decodedLen);
  memset(decoded, 0, decodedLen);
  free(decoded);
  return secret;
}
