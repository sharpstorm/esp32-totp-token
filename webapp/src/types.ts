export enum OtpAlgorithm {
  INVALID = 0,
  SHA1 = 1,
}

export enum OtpType {
  OTP_INVALID = 0,
  OTP_HOTP = 1,
  OTP_TOTP = 2,
}

export type OtpParameters = {
  secretBase32: string
  name: string
  issuer: string
}
