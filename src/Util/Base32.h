#ifndef Base32_h
#define Base32_h

#include "Arduino.h"
#include "stdint.h"

class Base32 {
 public:
  // Returns chars written (0 on failure). Caller frees `out`.
  static int toBase32(byte* in, long bitLength, byte*& out);
  // Returns decoded byte count (-1 on invalid input). Caller frees `out`.
  static int fromBase32(byte* in, long length, byte*& out);
};

#endif
