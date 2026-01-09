export const requireElement = <T extends HTMLElement>(id: string) => {
  const ele = document.getElementById(id)
  if (!ele) {
    throw new Error(`Could not bind element: ${id}`)
  }
  return ele as T
}

export const require = <T>(obj: T | null | undefined) => {
  if (obj === null || obj === undefined) {
    throw new Error('Expected to not be null or undefined')
  }
  return obj
}

export const previewVideo = requireElement<HTMLVideoElement>('preview-video')
export const startBtn = requireElement<HTMLButtonElement>('start-btn')
export const stopBtn = requireElement<HTMLButtonElement>('stop-btn')
export const resultBox = requireElement<HTMLDivElement>('result')
export const resultUrl = requireElement<HTMLDivElement>('result-url')
export const resultItemsContainer = requireElement<HTMLDivElement>(
  'result-items-container',
)
export const status = requireElement<HTMLDivElement>('status')
export const videoContainer = requireElement<HTMLDivElement>('video-container')

export const tabWifiConfig =
  requireElement<HTMLButtonElement>('tab-wifi-config')
export const tabSecretConfig =
  requireElement<HTMLButtonElement>('tab-secret-config')
export const contentPaneWifiConfig = requireElement<HTMLDivElement>(
  'wifi-config-content-container',
)
export const contentPaneSecretConfig = requireElement<HTMLDivElement>(
  'secret-config-content-container',
)
