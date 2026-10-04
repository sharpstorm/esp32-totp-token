#include "QrCode.h"

#define GRID(r, c) grid[(r) * W + (c)]

bool QrCode::tablesInitialized = false;
uint8_t QrCode::expTable[256];
uint8_t QrCode::logTable[256];

uint8_t QrCode::gfMul(uint8_t x, uint8_t y) {
  if (x == 0 || y == 0) return 0;
  return expTable[(logTable[x] + logTable[y]) % 255];
}

void QrCode::rsEncode(uint8_t* data, int dataLen, uint8_t* ec, int ecLen) {
  uint8_t gen[40] = {0};
  gen[ecLen - 1] = 1;
  for (int i = 0; i < ecLen; i++) {
    uint8_t root = expTable[i];
    for (int j = 0; j < ecLen; j++) {
      gen[j] = gfMul(gen[j], root);
      if (j + 1 < ecLen) gen[j] ^= gen[j + 1];
    }
  }
  memset(ec, 0, ecLen);
  for (int i = 0; i < dataLen; i++) {
    uint8_t factor = data[i] ^ ec[0];
    memmove(ec, ec + 1, ecLen - 1);
    ec[ecLen - 1] = 0;
    for (int j = 0; j < ecLen; j++) {
      ec[j] ^= gfMul(gen[j], factor);
    }
  }
}

uint8_t* QrCode::generate(const char* url, uint8_t version, int16_t req_size,
                          int16_t* out_size) {
  if (version < 2 || version > 6) return nullptr;

  // Version parameters: {dataCap, ecLen, maxLen, alignPos}
  // Low ECC, byte mode, single block (except v6 which uses 2 blocks)
  // Version 2: W=25, 34 data + 10 EC codewords, 1 block, alignment at (18,18)
  static const int params[5][4] = {
      {34, 10, 32, 18},    // Version 2
      {55, 15, 53, 22},    // Version 3
      {80, 20, 78, 26},    // Version 4
      {108, 26, 106, 30},  // Version 5
      {136, 36, 134, 34},  // Version 6
  };
  int idx = version - 2;
  int dataCap = params[idx][0];
  int ecLen = params[idx][1];
  int maxLen = params[idx][2];
  int alignPos = params[idx][3];

  int len = strlen(url);
  if (len > maxLen) return nullptr;  // URL too long for version

  if (!tablesInitialized) {
    uint8_t x = 1;
    for (int i = 0; i < 255; i++) {
      expTable[i] = x;
      logTable[x] = i;
      x = (x << 1) ^ ((x & 0x80) ? 0x11D : 0);
    }
    expTable[255] = 0;
    tablesInitialized = true;
  }

  uint8_t bitstream[200] = {0};
  int bitPos = 0;

  auto addBits = [&](int val, int count) {
    for (int i = count - 1; i >= 0; i--) {
      if ((val >> i) & 1) bitstream[bitPos / 8] |= (1 << (7 - (bitPos % 8)));
      bitPos++;
    }
  };

  // 1. Bitstream Generation
  addBits(0x4, 4);  // Byte mode
  addBits(len, 8);  // Character count
  for (int i = 0; i < len; i++) addBits(url[i], 8);
  addBits(0x0, 4);  // Terminator
  while (bitPos % 8 != 0) bitPos++;

  int padBytes[] = {0xEC, 0x11};
  int padIdx = 0;
  while (bitPos / 8 < dataCap) {
    addBits(padBytes[padIdx++ % 2], 8);
  }

  // 2. Interleaving & Error Correction
  uint8_t interleaved[200] = {0};
  int totalBits = 0;
  if (version <= 5) {
    // Versions 2-5: Single block
    uint8_t ec[26] = {0};
    rsEncode(bitstream, dataCap, ec, ecLen);
    memcpy(interleaved, bitstream, dataCap);
    memcpy(interleaved + dataCap, ec, ecLen);
    totalBits = (dataCap + ecLen) * 8;
  } else {
    // Version 6: 2 blocks of 68 data + 18 EC each
    uint8_t ec1[18] = {0}, ec2[18] = {0};
    rsEncode(bitstream, 68, ec1, 18);
    rsEncode(bitstream + 68, 68, ec2, 18);
    int p = 0;
    for (int i = 0; i < 68; i++) {
      interleaved[p++] = bitstream[i];
      interleaved[p++] = bitstream[68 + i];
    }
    for (int i = 0; i < 18; i++) {
      interleaved[p++] = ec1[i];
      interleaved[p++] = ec2[i];
    }
    totalBits = 172 * 8;
  }

  // 3. Matrix Placement
  int W = 17 + 4 * version;  // v3=29, v4=33, v5=37, v6=41
  int8_t* grid = (int8_t*)malloc(W * W);
  memset(grid, -1, W * W);

  // Finders
  auto drawFinder = [&](int r, int c) {
    for (int i = -1; i <= 7; i++) {
      for (int j = -1; j <= 7; j++) {
        if (r + i < 0 || r + i >= W || c + j < 0 || c + j >= W) continue;
        int val = 0;
        if (i == 0 || i == 6 || j == 0 || j == 6)
          val = 1;
        else if (i >= 2 && i <= 4 && j >= 2 && j <= 4)
          val = 1;
        else if (i == -1 || i == 7 || j == -1 || j == 7)
          val = 0;
        GRID(r + i, c + j) = val;
      }
    }
  };
  drawFinder(0, 0);
  drawFinder(0, W - 7);
  drawFinder(W - 7, 0);

  // Alignment
  auto drawAlignment = [&](int r, int c) {
    for (int i = -2; i <= 2; i++) {
      for (int j = -2; j <= 2; j++) {
        GRID(r + i, c + j) =
            (i == -2 || i == 2 || j == -2 || j == 2 || (i == 0 && j == 0)) ? 1
                                                                           : 0;
      }
    }
  };
  drawAlignment(alignPos, alignPos);

  // Timing & Dark Module
  for (int i = 8; i < W - 8; i++) {
    GRID(6, i) = GRID(i, 6) = (i % 2 == 0) ? 1 : 0;
  }
  GRID(W - 8, 8) = 1;

  // Format Info (Mask 0, Low ECC -> 0x77C4)
  int fmtCoords[15][4] = {{8, 0, W - 1, 8}, {8, 1, W - 2, 8}, {8, 2, W - 3, 8},
                          {8, 3, W - 4, 8}, {8, 4, W - 5, 8}, {8, 5, W - 6, 8},
                          {8, 7, W - 7, 8}, {8, 8, 8, W - 8}, {7, 8, 8, W - 7},
                          {5, 8, 8, W - 6}, {4, 8, 8, W - 5}, {3, 8, 8, W - 4},
                          {2, 8, 8, W - 3}, {1, 8, 8, W - 2}, {0, 8, 8, W - 1}};
  for (int i = 0; i < 15; i++) {
    int bit = (0x77C4 >> (14 - i)) & 1;
    GRID(fmtCoords[i][0], fmtCoords[i][1]) = bit;
    GRID(fmtCoords[i][2], fmtCoords[i][3]) = bit;
  }

  // Zigzag Data Placement + Mask 0
  int r = W - 1, c = W - 1, dir = -1, bitIndex = 0;
  while (c > 0) {
    if (c == 6) c--;
    for (int j = 0; j < 2; j++) {
      int col = c - j;
      if (GRID(r, col) == -1) {
        int bit = 0;
        if (bitIndex < totalBits) {
          bit = (interleaved[bitIndex / 8] >> (7 - (bitIndex % 8))) & 1;
          bitIndex++;
        }
        GRID(r, col) = bit ^ (((r + col) % 2 == 0) ? 1 : 0);
      }
    }
    r += dir;
    if (r < 0 || r >= W) {
      dir = -dir;
      r += dir;
      c -= 2;
    }
  }

  // 4. Transform into Scaled drawBitmap Buffer
  int actualW = W + 8;  // Including standard 4-module quiet zone padding
  int S = req_size / actualW;
  if (S < 1) S = 1;
  *out_size = actualW * S;

  int row_bytes = (*out_size + 7) / 8;
  uint8_t* bmp = (uint8_t*)calloc(*out_size, row_bytes);

  if (bmp) {
    for (int row = 0; row < W; row++) {
      for (int col = 0; col < W; col++) {
        if (GRID(row, col) == 1) {
          for (int dy = 0; dy < S; dy++) {
            for (int dx = 0; dx < S; dx++) {
              int px = (col + 4) * S + dx;  // +4 for quiet zone offset
              int py = (row + 4) * S + dy;
              bmp[py * row_bytes + (px / 8)] |=
                  (1 << (px % 8));  // XBM is LSB-first
            }
          }
        }
      }
    }
  }

  free(grid);  // Cleanup abstract matrix
  return bmp;
}