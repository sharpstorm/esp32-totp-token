#pragma once

#ifndef Secret_h
#define Secret_h

#include "Arduino.h"
#include <stdint.h>
#define byte uint8_t

class Secret {
  private:
    uint16_t secretBitLen;
    byte *secret;
    String name;

  public:
    Secret(uint16_t secretBitLen);
    Secret(const Secret &other);
    ~Secret();

    byte *get();
    const uint16_t bitLen();
    const uint8_t byteLen();
    bool isValid();
    void setName(String name);
    const String getName();
};

#endif
