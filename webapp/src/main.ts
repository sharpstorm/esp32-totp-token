import QrScanner from 'qr-scanner'
import {
  previewVideo,
  resultBox,
  startBtn,
  stopBtn,
  videoContainer,
  status,
  resultUrl,
  resultItemsContainer,
} from './bindings'
import { decodeOtpUrl } from './payload-parser'
import { makeNode } from './dom-utils'
import { bindContentTabInteraction } from './tabs'
import { bindManualSecretInput } from './secret-input'

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
    const { otpConfigs } = decodeOtpUrl(otpUrl)
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

document.addEventListener('DOMContentLoaded', () => {
  bindContentTabInteraction()
  bindManualSecretInput()
  startBtn.addEventListener('click', startScanning)
  stopBtn.addEventListener('click', stopScanning)
  qrScanner = new QrScanner(previewVideo, handleQrScanResult, {})
})
