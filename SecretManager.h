#pragma once

#ifndef SecretManager_h
#define SecretManager_h

#include <Preferences.h>

#define SECRET_NAMESPACE "totp-secrets"
#define SECRET_INDEX "Sindex"
#define SECRET_RECORD_PREFIX "SR-"
#define SECRET_NAME_PREFIX "SN-"

class SecretManager {
  private:
    Preferences preferences;

    bool isStarted = false;
    uint8_t secretCount = 0;

  public:
    bool start();
    void end();
    bool isActive();

    void readRecord(uint8_t index);
};

#endif
