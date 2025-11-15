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
  ~Secret();

  uint8_t* get();
  const uint16_t bitLen();
  const uint8_t byteLen();
  bool isValid();
  void setName(String name);
  const String getName();
};

#endif
