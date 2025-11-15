#include "TOTP.h"

#include "sha1.h"

// Init the library with the private key, its length and the timeStep duration
TOTP::TOTP(uint8_t* hmacKey, uint8_t keyLength, uint32_t timeStep) {
  _hmacKey = hmacKey;
  _keyLength = keyLength;
  _timeStep = timeStep;
}

// Generate a code, using the timestamp provided
char* TOTP::getCodeFromTimestamp(uint32_t timeStamp) {
  uint32_t steps = timeStamp / _timeStep;
  return getCodeFromSteps(steps);
}

// Generate a code, using the number of steps provided
char* TOTP::getCodeFromSteps(uint32_t steps) {
  // STEP 0, map the number of steps in a 8-bytes array (counter value)
  uint8_t _byteArray[8];
  _byteArray[0] = 0x00;
  _byteArray[1] = 0x00;
  _byteArray[2] = 0x00;
  _byteArray[3] = 0x00;
  _byteArray[4] = (uint8_t)((steps >> 24) & 0xFF);
  _byteArray[5] = (uint8_t)((steps >> 16) & 0xFF);
  _byteArray[6] = (uint8_t)((steps >> 8) & 0XFF);
  _byteArray[7] = (uint8_t)((steps & 0XFF));

  SHA1 shaEngine;
  // STEP 1, get the HMAC-SHA1 hash from counter and key
  shaEngine.initHmac(_hmacKey, _keyLength);
  shaEngine.writeArray(_byteArray, 8);
  uint8_t* _hash = shaEngine.resultHmac();

  // STEP 2, apply dynamic truncation to obtain a 4-bytes string
  uint32_t _truncatedHash = 0;
  uint8_t _offset = _hash[20 - 1] & 0xF;
  uint8_t j;
  for (j = 0; j < 4; ++j) {
    _truncatedHash <<= 8;
    _truncatedHash |= _hash[_offset + j];
  }

  // STEP 3, compute the OTP value
  _truncatedHash &= 0x7FFFFFFF;  // Disabled
  _truncatedHash %= 1000000;

  _code[0] = 48 + ((_truncatedHash / 100000) % 10);
  _code[1] = 48 + ((_truncatedHash / 10000) % 10);
  _code[2] = 48 + ((_truncatedHash / 1000) % 10);
  _code[3] = 48 + ((_truncatedHash / 100) % 10);
  _code[4] = 48 + ((_truncatedHash / 10) % 10);
  _code[5] = 48 + (_truncatedHash % 10);
  _code[6] = 0;

  return _code;
}
