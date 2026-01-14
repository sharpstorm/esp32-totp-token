import QrScanner from 'qr-scanner'
import { Api } from './api'
import {
  contentPaneSecretConfig,
  resultBox,
  secretConfigInputAddBtn,
  secretConfigInputName,
  secretConfigInputSecret,
  secretConfigManualButton,
  secretConfigScanButton,
  secretConfigSuccessCallout,
  startBtn,
  stopBtn,
  videoContainer,
  status,
  previewVideo,
  resultUrl,
  resultItemsContainer,
} from './bindings'
import { makeNode } from './dom-utils'
import { parseQrPayload } from './parsers/payload-parser'

export const bindSecretInputInteractions = () => {
  bindModeSelector()
  bindScanSecretInput()
  bindManualSecretInput()
}

const bindModeSelector = () => {
  secretConfigManualButton.addEventListener('click', () => {
    contentPaneSecretConfig.classList.remove('scan-mode')
    contentPaneSecretConfig.classList.add('input-mode')
    secretConfigScanButton.classList.remove('tab-active')
    secretConfigManualButton.classList.add('tab-active')
  })

  secretConfigScanButton.addEventListener('click', () => {
    contentPaneSecretConfig.classList.remove('input-mode')
    contentPaneSecretConfig.classList.add('scan-mode')
    secretConfigManualButton.classList.remove('tab-active')
    secretConfigScanButton.classList.add('tab-active')
  })
}

const bindManualSecretInput = () => {
  secretConfigInputAddBtn.addEventListener('click', () => {
    const secretName = secretConfigInputName.value
    const secretBase32 = secretConfigInputSecret.value

    if (!secretName || !secretBase32) {
      return
    }

    Api.saveSecret({ name: secretName, secret: secretBase32 }).then(() => {
      secretConfigInputName.value = ''
      secretConfigInputSecret.value = ''
      secretConfigSuccessCallout.classList.add('show')
      setTimeout(() => {
        secretConfigSuccessCallout.classList.remove('show')
      }, 5000)
    })
  })
}

const bindScanSecretInput = () => {
  startBtn.addEventListener('click', startScanning)
  stopBtn.addEventListener('click', stopScanning)
  qrScanner = new QrScanner(previewVideo, handleQrScanResult, {})

  secretConfigInputAddBtn.addEventListener('click', () => {
    const secretName = secretConfigInputName.value
    const secretBase32 = secretConfigInputSecret.value

    if (!secretName || !secretBase32) {
      return
    }

    Api.saveSecret({ name: secretName, secret: secretBase32 }).then(() => {
      secretConfigInputName.value = ''
      secretConfigInputSecret.value = ''
      secretConfigSuccessCallout.classList.add('show')
      setTimeout(() => {
        secretConfigSuccessCallout.classList.remove('show')
      }, 5000)
    })
  })
}

let qrScanner: QrScanner | null
let isScanning = false

const startScanning = async () => {
  if (isScanning) {
    return
  }

  qrScanner?.start()

  videoContainer.style.display = 'block'
  startBtn.style.display = 'none'
  stopBtn.style.display = 'block'
  resultBox.classList.remove('show')
  status.textContent = 'Camera active - Point at a QR code'

  isScanning = true
}

const stopScanning = async () => {
  if (!isScanning) {
    return
  }

  isScanning = false
  qrScanner?.stop()
  clearResults()
  videoContainer.style.display = 'none'
  startBtn.style.display = 'block'
  stopBtn.style.display = 'none'
  status.textContent = 'Camera stopped - Click "Start Camera" to scan again'
}

const handleQrScanResult = (result: QrScanner.ScanResult) => {
  clearResults()

  const otpUrl = result.data
  resultUrl.textContent = otpUrl
  resultBox.classList.add('show')
  status.textContent = 'QR Code found! Scanning continues...'

  // Vibrate if supported
  if (navigator.vibrate) {
    navigator.vibrate(200)
  }

  try {
    const otpConfigs = parseQrPayload(otpUrl)
    if (!otpConfigs) {
      status.textContent = 'Invalid QR'
      return
    }

    otpConfigs.forEach((config) => {
      addResultRow(config)
    })
  } catch (err) {}
}

const clearResults = () => {
  while (resultItemsContainer.lastChild) {
    resultItemsContainer.removeChild(resultItemsContainer.lastChild)
  }
  resultUrl.textContent = ''
}

const addResultRow = ({
  name,
  issuer,
  secretBase32,
}: {
  name: string
  issuer: string
  secretBase32: string
}) => {
  const row = makeNode(
    'div',
    { className: 'result-row' },
    makeNode(
      'div',
      { className: 'result-metadata' },
      makeNode('strong', {}, issuer),
      ` (${name})`,
    ),
    makeNode('div', { className: 'result-secret' }, secretBase32),
  )

  resultItemsContainer.appendChild(row)
}
