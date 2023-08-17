#include "SecretManager.h"

void SecretManager::start() {
  preferences.begin(SECRET_NAMESPACE, false);
  secretCount = preferences.getUChar(SECRET_INDEX, 0);
  isStarted = true;
}

void SecretManager::end() {
  preferences.end();
  isStarted = false;
  secretCount = 0;
}

bool SecretManager::isActive() {
  return isStarted;
}

void SecretManager::readRecord(uint8_t index) {
  /* String key = SECRET_RECORD_PREFIX + String(index);
  size_t secretSize = preferences.getBytesLength(key);
  byte *secret = malloc(secretSize);
  preferences.getBytes(key, secret, secretSize);

  
  free(secret); */
}
