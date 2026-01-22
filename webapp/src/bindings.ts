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

export const previewVideo = requireElement<HTMLVideoElement>(
  'secret-config-scan-preview-video',
)
export const startBtn = requireElement<HTMLButtonElement>(
  'secret-config-scan-start-btn',
)
export const stopBtn = requireElement<HTMLButtonElement>(
  'secret-config-scan-stop-btn',
)
export const status = requireElement<HTMLDivElement>(
  'secret-config-scan-status',
)
export const videoContainer = requireElement<HTMLDivElement>(
  'secret-config-scan-video-container',
)

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

export const secretConfigInputName =
  requireElement<HTMLInputElement>('input-secret-name')
export const secretConfigInputSecret = requireElement<HTMLInputElement>(
  'input-secret-base32',
)
export const secretConfigInputAddBtn = requireElement<HTMLButtonElement>(
  'btn-secret-input-add',
)
export const secretConfigSuccessCallout = requireElement<HTMLDivElement>(
  'input-secret-success',
)

export const secretConfigManualButton = requireElement<HTMLButtonElement>(
  'secret-config-manual-btn',
)
export const secretConfigScanButton = requireElement<HTMLButtonElement>(
  'secret-config-scan-btn',
)

export const currentSsidDiv =
  requireElement<HTMLDivElement>('wifi-current-ssid')
export const scanBtn = requireElement<HTMLButtonElement>('btn-wifi-scan')
export const networksListContainer =
  requireElement<HTMLDivElement>('wifi-networks-list')!
export const passphraseContainer = requireElement<HTMLDivElement>(
  'wifi-passphrase-input',
)!
export const passphraseInput = requireElement<HTMLInputElement>(
  'input-wifi-passphrase',
) as HTMLInputElement
export const selectedSsidDiv =
  requireElement<HTMLSpanElement>('wifi-selected-ssid')
export const saveBtn = requireElement<HTMLButtonElement>('btn-wifi-save')
export const successCallout =
  requireElement<HTMLDivElement>('wifi-save-success')

export const modal = requireElement<HTMLDivElement>('modal')
export const modalTitle = requireElement<HTMLTitleElement>('modal-title')
export const modalBody = requireElement<HTMLDivElement>('modal-body')
export const modalCloseBtn =
  requireElement<HTMLButtonElement>('modal-close-btn')
export const modalCancelBtn =
  requireElement<HTMLButtonElement>('modal-cancel-btn')
export const modalSaveBtn = requireElement<HTMLButtonElement>('modal-save-btn')

export const secretsManageBtn = requireElement<HTMLButtonElement>(
  'secret-config-manage-btn',
)
export const secretsRefreshBtn = requireElement<HTMLButtonElement>(
  'secret-config-manage-refresh-btn',
)
export const secretsList = requireElement<HTMLDivElement>(
  'secret-config-manage-list',
)
export const secretsUpdateStatus = requireElement<HTMLDivElement>(
  'secret-config-manage-status',
)
