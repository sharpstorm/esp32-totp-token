import { encodeBase32 } from './base32'
import { ReadableStream } from '../readable-stream'
import { OtpParameters } from '../types'

export const decodeGoogleAuthenticatorUrl = (url: string) => {
  const base64Data = getBase64Data(url)
  if (!base64Data) {
    throw new Error('Not a valid Google Authenticator URL')
  }

  const arrayBuf = base64ToUInt8Array(base64Data)
  return readProtobuf(arrayBuf)
}

const getBase64Data = (url: string) => {
  try {
    const urlObject = new URL(url)
    if (urlObject.protocol !== 'otpauth-migration:') {
      return null
    } else if (urlObject.host !== 'offline') {
      return null
    }

    return urlObject.searchParams.get('data')
  } catch (err) {
    return null
  }
}

const base64ToUInt8Array = (base64: string) => {
  var binaryString = atob(base64)
  var bytes = new Uint8Array(binaryString.length)
  for (var i = 0; i < binaryString.length; i++) {
    bytes[i] = binaryString.charCodeAt(i)
  }
  return bytes
}

/*
Protobuf V3:

message MigrationPayload {
  enum Algorithm {
    ALGO_INVALID = 0;
    ALGO_SHA1 = 1;
  }

  enum OtpType {
    OTP_INVALID = 0;
    OTP_HOTP = 1;
    OTP_TOTP = 2;
  }

  message OtpParameters {
    bytes secret = 1;
    string name = 2;
    string issuer = 3;
    Algorithm algorithm = 4;
    int32 digits = 5;
    OtpType type = 6;
    int64 counter = 7;
  }

  repeated OtpParameters otp_parameters = 1;
  int32 version = 2;
  int32 batch_size = 3;
  int32 batch_index = 4;
  int32 batch_id = 5;
}
*/

type MigrationMetadata = {
  version?: number
  batchSize?: number
  batchIndex?: number
  batchId?: number
}

enum ProtobufMetadataFields {
  Version = 2,
  BatchSize = 3,
  BatchIndex = 4,
  BatchId = 5,
}

enum ProtobufOtpParametersFields {
  Secret = 1,
  Name = 2,
  Issuer = 3,
  Algorithm = 4,
  Digits = 5,
  OtpType = 6,
  Counter = 7,
}

enum ProtobufSectionType {
  VARINT = 0,
  FIXED64 = 1,
  LENDELIM = 2,
  FIXED32 = 5,
}

// Based on https://github.com/pawitp/protobuf-decoder?tab=readme-ov-file
const readProtobuf = (buf: Uint8Array) => {
  const readStream = new ReadableStream(buf)
  const metadata: MigrationMetadata = {}
  const otpConfigs: OtpParameters[] = []

  // Read OtpParameters structs
  while (readStream.available() > 0) {
    const { fieldId, type } = readProtobufSectionHeader(readStream)
    if (fieldId === 1) {
      assertType(type, ProtobufSectionType.LENDELIM)
      const data = readProtobufLenDelim(readStream)
      otpConfigs.push(decodeOtpParametersStruct(data))
    } else if (
      fieldId >= ProtobufMetadataFields.Version &&
      fieldId <= ProtobufMetadataFields.BatchId
    ) {
      assertType(type, ProtobufSectionType.VARINT)
      const value = Number(readStream.readVarInt())

      if (fieldId === ProtobufMetadataFields.Version) {
        metadata.version = value
      } else if (fieldId === ProtobufMetadataFields.BatchId) {
        metadata.batchId = value
      } else if (fieldId === ProtobufMetadataFields.BatchIndex) {
        metadata.batchIndex = value
      } else if (fieldId === ProtobufMetadataFields.BatchSize) {
        metadata.batchSize = value
      }
    } else {
      throw new Error('Unknown protobuf fieldId')
    }
  }

  return { metadata, otpConfigs }
}

const readProtobufSectionHeader = (readStream: ReadableStream) => {
  const fieldIdAndType = parseInt(readStream.readVarInt().toString())
  const type = fieldIdAndType & 0b111
  const fieldId = fieldIdAndType >> 3

  return { type, fieldId }
}

const readProtobufLenDelim = (readStream: ReadableStream) => {
  const length = parseInt(readStream.readVarInt().toString())
  const data = readStream.read(length)
  return data
}

const assertType = (type: number, expectedType: ProtobufSectionType) => {
  if (type !== expectedType) {
    throw new Error('Malformed protobuf struct - Unexpected type')
  }
}

const textDecoder = new TextDecoder()
const decodeOtpParametersStruct = (data: Uint8Array): OtpParameters => {
  let secret: string | undefined
  let name: string | undefined
  let issuer: string | undefined

  const readStream = new ReadableStream(data)
  while (readStream.available() > 0) {
    const { fieldId, type } = readProtobufSectionHeader(readStream)

    if (fieldId === ProtobufOtpParametersFields.Secret) {
      assertType(type, ProtobufSectionType.LENDELIM)
      const data = readProtobufLenDelim(readStream)
      secret = encodeBase32(data)
    } else if (fieldId === ProtobufOtpParametersFields.Name) {
      assertType(type, ProtobufSectionType.LENDELIM)
      const data = readProtobufLenDelim(readStream)
      name = textDecoder.decode(data)
    } else if (fieldId === ProtobufOtpParametersFields.Issuer) {
      assertType(type, ProtobufSectionType.LENDELIM)
      const data = readProtobufLenDelim(readStream)
      issuer = textDecoder.decode(data)

      // For other cases, drain the buffer
    } else if (type === ProtobufSectionType.LENDELIM) {
      readProtobufLenDelim(readStream)
    } else if (type === ProtobufSectionType.VARINT) {
      readStream.readVarInt()
    }
  }

  if (!secret || !name || !issuer) {
    throw new Error('Malformed OtpParameters - missing field')
  }

  return {
    secretBase32: secret,
    name,
    issuer,
  }
}
