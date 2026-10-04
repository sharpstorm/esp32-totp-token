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
  // Returns false if the store is full (count is a uint8_t) or invalid.
  bool putRecord(Secret* secret);
  bool deleteRecord(uint8_t index);
  // Rename and/or replace the secret at `index`. If `secret` is null only
  // the name is changed. Returns false on an invalid index or empty name.
  bool updateRecord(uint8_t index, const String& name, Secret* secret);
  int getSecretCount();
  bool isIndexValid(uint8_t index);
};

extern SecretManager secretManager;

#endif
