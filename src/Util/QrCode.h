#pragma once
#include <Arduino.h>

class QrCode {
 public:
  static uint8_t* generate(const char* url, uint8_t version, int16_t req_size,
                           int16_t* out_size);

 private:
  static uint8_t gfMul(uint8_t x, uint8_t y);
  static void rsEncode(uint8_t* data, int dataLen, uint8_t* ec, int ecLen);

  static bool tablesInitialized;
  static uint8_t expTable[256];
  static uint8_t logTable[256];
};
