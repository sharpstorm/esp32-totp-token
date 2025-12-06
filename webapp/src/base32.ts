const alphabet = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ234567'

export const encodeBase32 = (data: Uint8Array) => {
  let skip = 0 // how many bits we will skip from the first byte
  let bits = 0 // 5 high bits, carry from one byte to the next

  let output = ''

  for (let i = 0; i < data.length; ) {
    const byte = data[i]

    if (skip < 0) {
      // we have a carry from the previous byte
      bits |= byte >> -skip
    } else {
      // no carry
      bits = (byte << skip) & 248
    }

    if (skip > 3) {
      // not enough data to produce a character, get us another one
      skip -= 8
      i++
      continue
    }

    if (skip < 4) {
      // produce a character
      output += alphabet[bits >> 3]
      skip += 5
    }
  }

  return output + (skip < 0 ? alphabet[bits >> 3] : '')
}
