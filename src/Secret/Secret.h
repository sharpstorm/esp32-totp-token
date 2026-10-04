#pragma once

#ifndef Secret_h
#define Secret_h

#include <stdint.h>

#include "Arduino.h"

class Secret {
 private:
  uint16_t secretBitLen;
  uint8_t* secret;
  String name;

 public:
  Secret(uint16_t secretBitLen);
  Secret(const Secret& other);
  Secret& operator=(const Secret& other) = delete;
  ~Secret();

  // Decodes a base32 secret. The secret length is taken from the number of
  // bytes actually decoded (padding/whitespace ignored), never from the input
  // character count. Returns an invalid Secret (bitLen 0) on bad input.
  static Secret fromBase32(const uint8_t* base32, size_t len);

  uint8_t* get();
  const uint16_t bitLen();
  const uint8_t byteLen();
  bool isValid();
  void setName(String name);
  const String getName();
};

#endif
