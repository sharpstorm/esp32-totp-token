#pragma once

#include "Arduino.h"

#define SECRET_RECORD_PREFIX "SR-"
#define SECRET_SIZE_PREFIX "SS-"
#define SECRET_NAME_PREFIX "SN-"

class StoredKey {
 private:
  String sizeKey;
  String secretKey;
  String nameKey;

 public:
  StoredKey();
  StoredKey(const String& index);
  const char* cSizeKey();
  const char* cSecretKey();
  const char* cNameKey();
};