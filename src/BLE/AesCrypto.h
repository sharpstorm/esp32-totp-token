#pragma once

#ifndef AesCrypto_h
#define AesCrypto_h

#include <stddef.h>
#include <stdint.h>

#define AES_KEY_LEN 16
#define AES_IV_LEN 12
#define AES_TAG_LEN 16

// Wire protocol version, bound into every packet's AAD.
#define AES_PROTO_VERSION 0x02

// Direction byte, bound into every packet's AAD. Prevents a packet sent by
// one side from being reflected back and accepted by the same side.
#define AES_DIR_CLIENT_TO_DEVICE 0x01
#define AES_DIR_DEVICE_TO_CLIENT 0x02

// Encrypted packet layout (AES-128-GCM):
//   [12 bytes IV] [N bytes ciphertext] [16 bytes GCM tag]
//   IV  = 8 random bytes || 4-byte big-endian message counter
//   AAD = [AES_PROTO_VERSION, direction]
// Each sender starts its counter at 0 for a new key and increments it by 1
// per message. The receiver rejects any counter <= the last accepted one
// (anti-replay). Overhead per packet = 28 bytes.

class AesCrypto {
 private:
  uint8_t key[AES_KEY_LEN];
  uint32_t txCounter;
  int64_t lastRxCounter;  // -1 = nothing received yet under this key

 public:
  AesCrypto();

  void setKey(const uint8_t* newKey);
  void getKey(uint8_t* outKey);
  void generateRandomKey();
  void reset();

  // Encrypt a device->client packet.
  // Returns total encrypted packet length, or -1 on failure.
  int encrypt(const uint8_t* plaintext, size_t plaintextLen, uint8_t* outBuf,
              size_t outBufLen);

  // Decrypt and authenticate a client->device packet, enforcing a strictly
  // increasing counter. Returns plaintext length, or -1 on failure.
  int decrypt(const uint8_t* packet, size_t packetLen, uint8_t* outBuf,
              size_t outBufLen);

  // Cryptographically random bytes. Only truly random while the RF
  // subsystem (BT or Wi-Fi) is enabled.
  static void randomBytes(uint8_t* buf, size_t len);

  // Format the key as a hex string (32 chars + null)
  void keyToHex(char* hexBuf);
};

#endif
