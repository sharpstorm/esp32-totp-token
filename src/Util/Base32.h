#ifndef Base32_h
#define Base32_h

#include "Arduino.h"
#include "stdint.h"

class Base32 {
 public:
  static int toBase32(byte*, long, byte*&);
  static int fromBase32(byte*, long, byte*&);
};

#endif
