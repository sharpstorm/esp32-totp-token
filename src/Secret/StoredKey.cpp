#include "StoredKey.h"

StoredKey::StoredKey(const String& index)
    : sizeKey(SECRET_SIZE_PREFIX + index),
      secretKey(SECRET_RECORD_PREFIX + index),
      nameKey(SECRET_NAME_PREFIX + index) {}

StoredKey::StoredKey()
    : sizeKey(String()), secretKey(String()), nameKey(String()) {}

const char* StoredKey::cSizeKey() { return sizeKey.c_str(); }

const char* StoredKey::cSecretKey() { return secretKey.c_str(); }

const char* StoredKey::cNameKey() { return nameKey.c_str(); }