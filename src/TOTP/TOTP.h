#pragma once

#ifndef TOTP_H
#define TOTP_H

#include <inttypes.h>

#include "time.h"

class TOTP {
 private:
  uint8_t* _hmacKey;
  uint8_t _keyLength;
  uint32_t _timeStep;
  char _code[7];

 public:
  TOTP(uint8_t* hmacKey, uint8_t keyLength, uint32_t timeStep);
  char* getCodeFromTimestamp(uint32_t timeStamp);
  char* getCodeFromSteps(uint32_t steps);
};

#endif
