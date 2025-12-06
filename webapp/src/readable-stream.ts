const decodeVarint = (buffer: Uint8Array, offset: number) => {
  let res = BigInt(0)
  let shift = 0
  let byte = 0

  do {
    if (offset >= buffer.length) {
      throw new RangeError('Index out of bound decoding varint')
    }

    byte = buffer[offset++]

    const multiplier = BigInt(2) ** BigInt(shift)
    const thisByteValue = BigInt(byte & 0x7f) * multiplier
    shift += 7
    res = res + thisByteValue
  } while (byte >= 0x80)

  return {
    value: res,
    length: shift / 7,
  }
}

export class ReadableStream {
  private readonly buffer: Uint8Array
  offset: number

  constructor(buffer: Uint8Array) {
    this.buffer = buffer
    this.offset = 0
  }

  readVarInt() {
    const result = decodeVarint(this.buffer, this.offset)
    this.offset += result.length

    return result.value
  }

  read(length: number) {
    this.checkByte(length)
    const result = this.buffer.slice(this.offset, this.offset + length)
    this.offset += length

    return result
  }

  available() {
    return this.buffer.length - this.offset
  }

  private checkByte(length: number) {
    const bytesAvailable = this.available()
    if (length > bytesAvailable) {
      throw new Error(
        'Not enough bytes left. Requested: ' +
          length +
          ' left: ' +
          bytesAvailable,
      )
    }
  }
}
