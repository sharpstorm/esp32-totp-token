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

export type AddPrefix<TKey, TPrefix extends string> = TKey extends string
  ? `${TPrefix}${Capitalize<TKey>}`
  : never

export type AddPrefixToObject<
  TObject extends object,
  TPrefix extends string,
> = {
  [K in keyof TObject as AddPrefix<K, TPrefix>]: TObject[K]
}
