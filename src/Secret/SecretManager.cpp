#include "SecretManager.h"

void SecretManager::start() {
  if (isStarted) {
    return;
  }

  preferences.begin(SECRET_NAMESPACE, false);
  secretCount = preferences.getUChar(SECRET_INDEX, 0);
  isStarted = true;
}

void SecretManager::end() {
  if (!isStarted) {
    return;
  }

  preferences.end();
  isStarted = false;
  secretCount = 0;
}

bool SecretManager::isActive() { return isStarted; }

void SecretManager::clear() { preferences.clear(); }

bool SecretManager::isIndexValid(uint8_t index) {
  return index >= 0 && index < secretCount;
}

Secret SecretManager::readRecord(uint8_t index) {
  if (!isIndexValid(index)) {
    return Secret(0);
  }

  StoredKey keyNames = StoredKey(String(index));
  uint16_t secretBitLen = preferences.getUShort(keyNames.cSizeKey());
  Secret secret = Secret(secretBitLen);
  size_t bytesRead = preferences.getBytes(keyNames.cSecretKey(), secret.get(),
                                          secret.byteLen());
  if (bytesRead != secret.byteLen()) {
    Serial.println("Failed to read secret");
    return Secret(0);
  }
  secret.setName(preferences.getString(keyNames.cNameKey()));

  return secret;
}

void SecretManager::putRecord(Secret* secret) {
  StoredKey keyNames = StoredKey(String(secretCount));

  preferences.putUShort(keyNames.cSizeKey(), secret->bitLen());
  preferences.putBytes(keyNames.cSecretKey(), secret->get(), secret->byteLen());
  preferences.putString(keyNames.cNameKey(), secret->getName());
  secretCount++;
  preferences.putUChar(SECRET_INDEX, secretCount);
}

bool SecretManager::deleteRecord(uint8_t index) {
  StoredKey targetKeyNames;
  StoredKey sourceKeyNames;

  size_t secretSize;
  uint8_t* secret;
  int curIdx = index;

  if (!isIndexValid(index)) {
    return false;
  }

  for (; curIdx < secretCount - 1; curIdx++) {
    targetKeyNames = StoredKey(String(curIdx));
    sourceKeyNames = StoredKey(String(curIdx + 1));

    secretSize = preferences.getBytesLength(sourceKeyNames.cSecretKey());
    secret = (uint8_t*)malloc(secretSize);
    preferences.getBytes(sourceKeyNames.cSecretKey(), secret, secretSize);
    preferences.putBytes(targetKeyNames.cSecretKey(), secret, secretSize);
    free(secret);

    uint16_t bitLen = preferences.getUShort(sourceKeyNames.cSizeKey());
    preferences.putUShort(targetKeyNames.cSizeKey(), bitLen);

    String name = preferences.getString(sourceKeyNames.cNameKey());
    preferences.putString(targetKeyNames.cNameKey(), name);
  }

  targetKeyNames = StoredKey(String(curIdx));
  preferences.remove(targetKeyNames.cSecretKey());
  preferences.remove(targetKeyNames.cSizeKey());
  preferences.remove(targetKeyNames.cNameKey());

  secretCount--;
  preferences.putUChar(SECRET_INDEX, secretCount);

  return true;
}

int SecretManager::getSecretCount() { return secretCount; }
