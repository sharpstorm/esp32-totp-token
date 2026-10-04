#include "AesCrypto.h"

#include <string.h>

#include "esp_system.h"
#include "mbedtls/gcm.h"

AesCrypto::AesCrypto() : txCounter(0), lastRxCounter(-1) {
  memset(key, 0, AES_KEY_LEN);
}

void AesCrypto::setKey(const uint8_t* newKey) {
  memcpy(key, newKey, AES_KEY_LEN);
  txCounter = 0;
  lastRxCounter = -1;
}

void AesCrypto::getKey(uint8_t* outKey) { memcpy(outKey, key, AES_KEY_LEN); }

void AesCrypto::generateRandomKey() {
  randomBytes(key, AES_KEY_LEN);
  txCounter = 0;
  lastRxCounter = -1;
}

void AesCrypto::reset() {
  memset(key, 0, AES_KEY_LEN);
  txCounter = 0;
  lastRxCounter = -1;
}

void AesCrypto::randomBytes(uint8_t* buf, size_t len) {
  esp_fill_random(buf, len);
}

int AesCrypto::encrypt(const uint8_t* plaintext, size_t plaintextLen,
                       uint8_t* outBuf, size_t outBufLen) {
  size_t totalLen = AES_IV_LEN + plaintextLen + AES_TAG_LEN;
  if (outBufLen < totalLen || txCounter == UINT32_MAX) {
    return -1;
  }

  // IV: 8 random bytes + 4-byte big-endian counter
  uint8_t iv[AES_IV_LEN];
  randomBytes(iv, 8);
  iv[8] = (txCounter >> 24) & 0xFF;
  iv[9] = (txCounter >> 16) & 0xFF;
  iv[10] = (txCounter >> 8) & 0xFF;
  iv[11] = txCounter & 0xFF;
  txCounter++;

  memcpy(outBuf, iv, AES_IV_LEN);

  const uint8_t aad[2] = {AES_PROTO_VERSION, AES_DIR_DEVICE_TO_CLIENT};

  mbedtls_gcm_context gcm;
  mbedtls_gcm_init(&gcm);

  int ret = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 128);
  if (ret != 0) {
    mbedtls_gcm_free(&gcm);
    return -1;
  }

  uint8_t* ciphertext = outBuf + AES_IV_LEN;
  uint8_t* tag = outBuf + AES_IV_LEN + plaintextLen;

  ret = mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, plaintextLen, iv,
                                  AES_IV_LEN, aad, sizeof(aad), plaintext,
                                  ciphertext, AES_TAG_LEN, tag);
  mbedtls_gcm_free(&gcm);

  return ret == 0 ? (int)totalLen : -1;
}

int AesCrypto::decrypt(const uint8_t* packet, size_t packetLen,
                       uint8_t* outBuf, size_t outBufLen) {
  if (packetLen < AES_IV_LEN + AES_TAG_LEN) {
    return -1;
  }

  size_t ciphertextLen = packetLen - AES_IV_LEN - AES_TAG_LEN;
  if (outBufLen < ciphertextLen) {
    return -1;
  }

  const uint8_t* iv = packet;
  const uint8_t* ciphertext = packet + AES_IV_LEN;
  const uint8_t* tag = packet + AES_IV_LEN + ciphertextLen;

  const uint32_t counter = ((uint32_t)iv[8] << 24) | ((uint32_t)iv[9] << 16) |
                           ((uint32_t)iv[10] << 8) | (uint32_t)iv[11];
  // Cheap pre-check; the counter is only committed after authentication.
  if ((int64_t)counter <= lastRxCounter) {
    return -1;  // Replayed or reordered packet
  }

  const uint8_t aad[2] = {AES_PROTO_VERSION, AES_DIR_CLIENT_TO_DEVICE};

  mbedtls_gcm_context gcm;
  mbedtls_gcm_init(&gcm);

  int ret = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 128);
  if (ret != 0) {
    mbedtls_gcm_free(&gcm);
    return -1;
  }

  ret = mbedtls_gcm_auth_decrypt(&gcm, ciphertextLen, iv, AES_IV_LEN, aad,
                                 sizeof(aad), tag, AES_TAG_LEN, ciphertext,
                                 outBuf);
  mbedtls_gcm_free(&gcm);

  if (ret != 0) {
    memset(outBuf, 0, ciphertextLen);
    return -1;  // Authentication failed
  }

  lastRxCounter = counter;
  return (int)ciphertextLen;
}

void AesCrypto::keyToHex(char* hexBuf) {
  const char hexChars[] = "0123456789ABCDEF";
  for (int i = 0; i < AES_KEY_LEN; i++) {
    hexBuf[i * 2] = hexChars[(key[i] >> 4) & 0x0F];
    hexBuf[i * 2 + 1] = hexChars[key[i] & 0x0F];
  }
  hexBuf[AES_KEY_LEN * 2] = '\0';
}
