#include <inttypes.h>

#define HASH_LENGTH 20
#define BLOCK_LENGTH 64

#ifndef SHA1_H
#define SHA1_H

class SHA1 {
  private:
    union _buffer {
      uint8_t b[BLOCK_LENGTH];
      uint32_t w[BLOCK_LENGTH/4];
    } buffer;
    union _state {
      uint8_t b[HASH_LENGTH];
      uint32_t w[HASH_LENGTH/4];
    } state;

    uint8_t bufferOffset;
    uint32_t byteCount;
    uint8_t keyBuffer[BLOCK_LENGTH];
    uint8_t innerHash[HASH_LENGTH];

    uint32_t rol32(uint32_t number, uint8_t bits);
    void hashBlock();
    void addUncounted(uint8_t data);
    void pad();

  public:
    void resetState();
    void initHmac(const uint8_t* secret, uint8_t secretLength);
    uint8_t* result();
    uint8_t* resultHmac();
    void write(uint8_t);
    void writeArray(uint8_t *buffer, uint8_t size);
};

#endif
