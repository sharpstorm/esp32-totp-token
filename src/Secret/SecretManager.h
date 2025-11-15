#pragma once

#ifndef SecretManager_h
#define SecretManager_h

#include <Preferences.h>

#include "Secret.h"
#include "StoredKey.h"

#define SECRET_NAMESPACE "totp-secrets"
#define SECRET_INDEX "Sindex"

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
  void putRecord(Secret* secret);
  bool deleteRecord(uint8_t index);
  int getSecretCount();
  bool isIndexValid(uint8_t index);
};

#endif
