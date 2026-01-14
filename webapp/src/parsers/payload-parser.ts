import { OtpParameters } from '../types'
import { decodeGoogleAuthenticatorUrl } from './google-authenticator-parser'
import { decodeOtpAuthUrl } from './otpurl-parser'

export const parseQrPayload = (url: string): OtpParameters[] | null => {
  try {
    return decodeGoogleAuthenticatorUrl(url).otpConfigs
  } catch {}

  try {
    return decodeOtpAuthUrl(url)
  } catch {}

  return null
}
