import { OtpParameters } from '../types'

export const decodeOtpAuthUrl = (url: string): OtpParameters[] => {
  const data = getSecret(url)
  if (!data) {
    throw new Error('Not a valid OTP Auth URL')
  }

  return [data]
}

const getSecret = (url: string) => {
  try {
    const urlObject = new URL(url)
    if (urlObject.protocol !== 'otpauth:') {
      return null
    } else if (urlObject.host !== 'totp') {
      return null
    }

    const secretBase32 = urlObject.searchParams.get('secret')
    const issuer = urlObject.pathname.substring(1).split(':')[0]
    const name = urlObject.pathname.substring(1).split(':')[1]

    if (!secretBase32) {
      return null
    }

    return { secretBase32, issuer, name }
  } catch (err) {
    return null
  }
}
