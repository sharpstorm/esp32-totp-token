#pragma once

#ifndef SecretManager_h
#define SecretManager_h

#include <Preferences.h>
#include "Secret.h"

#define SECRET_NAMESPACE "totp-secrets"
#define SECRET_INDEX "Sindex"
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
    StoredKey(const String &index);
    const char* cSizeKey();
    const char* cSecretKey();
    const char* cNameKey();
};

class SecretManager {
  private:
    Preferences preferences;

    bool isStarted = false;
    uint8_t secretCount = 0;

  public:
    void start();
    void end();
    bool isActive();
    void clear();

    Secret readRecord(uint8_t index);
    void putRecord(Secret *secret);
    bool deleteRecord(uint8_t index);
    int getSecretCount();
    bool isIndexValid(uint8_t index);
};

#endif
