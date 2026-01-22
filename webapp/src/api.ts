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

export type WifiNetwork = {
  ssid: string
  rssi: number
  security: string
}

export type WifiNetworksDto = {
  networks: WifiNetwork[]
}

export type CurrentWifiDto = {
  ssid: string | null
}

export type SaveWifiReq = {
  ssid: string
  passphrase: string
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

const deleteSecret = async (index: number) => {
  const resp = await fetch(`/secrets?index=${index}`, {
    method: 'DELETE',
  })

  return resp.status === 200
}

const getCurrentWifi = async (): Promise<string | null> => {
  const resp = await fetch('/wifi/config')

  if (resp.status !== 200) {
    return null
  }

  const body: CurrentWifiDto = await resp.json()
  return body.ssid
}

const scanWifiNetworks = async (): Promise<WifiNetwork[]> => {
  const resp = await fetch('/wifi/scan', {
    method: 'POST',
  })

  if (resp.status !== 200) {
    return []
  }

  const body: WifiNetworksDto = await resp.json()
  return body.networks
}

const saveWifi = async ({ ssid, passphrase }: SaveWifiReq) => {
  const resp = await fetch('/wifi/config', {
    method: 'PUT',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({ ssid, passphrase }),
  })

  return resp.status === 200
}

export const Api = {
  saveSecret,
  getSecrets,
  deleteSecret,
  getCurrentWifi,
  scanWifiNetworks,
  saveWifi,
}
