#include "Base32.h"

// Encodes `bitLength` bits from `in` as RFC 4648 base32 (no '=' padding).
// The caller owns `out` (NUL-terminated) and must free() it.
// Returns the number of characters written, or 0 on failure (out = nullptr).
int Base32::toBase32(byte* in, long bitLength, byte*& out) {
  static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
  out = nullptr;

  if (in == nullptr || bitLength <= 0 || bitLength > 268435456LL) {
    return 0;
  }

  // Round up: a trailing partial group still needs a character.
  const long size = (bitLength + 4) / 5;
  const long inByteLen = (bitLength + 7) / 8;

  out = (byte*)malloc(size + 1);
  if (out == nullptr) {
    return 0;
  }

  uint32_t buffer = in[0];
  long next = 1;
  int bitsLeft = 8;
  for (long i = 0; i < size; i++) {
    if (bitsLeft < 5) {
      // Only pull more bytes while there is input left; otherwise zero-pad.
      if (next < inByteLen) {
        buffer = (buffer << 8) | (in[next++] & 0xFF);
        bitsLeft += 8;
      } else {
        int pad = 5 - bitsLeft;
        buffer <<= pad;
        bitsLeft += pad;
      }
    }
    int index = 0x1F & (buffer >> (bitsLeft - 5));
    bitsLeft -= 5;
    buffer &= (1u << bitsLeft) - 1;  // keep only unconsumed bits
    out[i] = (byte)alphabet[index];
  }
  out[size] = 0;

  return (int)size;
}

// Decodes base32 (case-insensitive; whitespace, '-' and '=' are ignored).
// The caller owns `out` and must free() it.
// Returns the number of decoded bytes, or -1 on invalid input (out = nullptr).
int Base32::fromBase32(byte* in, long length, byte*& out) {
  out = nullptr;
  if (in == nullptr || length < 0) {
    return -1;
  }

  int result = 0;
  uint32_t buffer = 0;
  int bitsLeft = 0;

  // Decoded output is never larger than the input.
  byte* temp = (byte*)malloc(length > 0 ? length : 1);
  if (temp == nullptr) {
    return -1;
  }

  for (long i = 0; i < length; i++) {
    byte ch = in[i];

    // Ignore separators and padding: ' ', NBSP, '\t', '\n', '\r', '-', '='
    if (ch == 0x20 || ch == 0xA0 || ch == 0x09 || ch == 0x0A || ch == 0x0D ||
        ch == 0x2D || ch == 0x3D)
      continue;

    // Recover common typos: '0' -> 'O', '1' -> 'L', '8' -> 'B'
    if (ch == 0x30) {
      ch = 0x4F;
    } else if (ch == 0x31) {
      ch = 0x4C;
    } else if (ch == 0x38) {
      ch = 0x42;
    }

    if ((ch >= 0x41 && ch <= 0x5A) || (ch >= 0x61 && ch <= 0x7A)) {
      ch = ((ch & 0x1F) - 1);
    } else if (ch >= 0x32 && ch <= 0x37) {
      ch -= (0x32 - 26);
    } else {
      free(temp);
      return -1;
    }

    buffer = (buffer << 5) | ch;
    bitsLeft += 5;
    if (bitsLeft >= 8) {
      temp[result++] = (byte)((buffer >> (bitsLeft - 8)) & 0xFF);
      bitsLeft -= 8;
      buffer &= (1u << bitsLeft) - 1;
    }
  }

  out = temp;
  return result;
}
