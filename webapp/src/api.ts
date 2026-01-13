export type SaveSecretOpts = {
  name: string
  secret: string
}

export type MaskedSecretDto = {
  name: string
}

export type SecretsPreviewDto = {
  secrets: MaskedSecretDto[]
  count: number
}

const saveSecret = async ({ name, secret }: SaveSecretOpts) => {
  const resp = await fetch('/secrets', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({ name, secret }),
  })

  return resp.status === 200
}

const getSecrets = async () => {
  const resp = await fetch('/secrets', {
    method: 'GET',
  })

  if (resp.status !== 200) {
    return []
  }

  const body: SecretsPreviewDto = await resp.json()
  return body.secrets
}

export const Api = { saveSecret, getSecrets }
