import QrScanner from 'qr-scanner'
import { Api } from './api'
import {
  contentPaneSecretConfig,
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
  modalSaveBtn,
  modalCancelBtn,
  modalCloseBtn,
  modal,
  modalTitle,
  modalBody,
} from './bindings'
import { makeNode, removeAllChildren } from './dom-utils'
import { parseQrPayload } from './parsers/payload-parser'
import { OtpParameters } from './types'

export const bindSecretInputInteractions = () => {
  bindModeSelector()
  bindScanSecretInput()
  bindManualSecretInput()
  bindModal()
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

let qrScanner: QrScanner | null
let isScanning = false

const bindScanSecretInput = () => {
  startBtn.addEventListener('click', startScanning)
  stopBtn.addEventListener('click', stopScanning)
  qrScanner = new QrScanner(previewVideo, handleQrScanResult, {})
}

const startScanning = async () => {
  if (isScanning) {
    return
  }

  qrScanner?.start()

  videoContainer.style.display = 'block'
  startBtn.style.display = 'none'
  stopBtn.style.display = 'block'
  status.textContent = 'Camera active - Point at a QR code'

  isScanning = true
}

const stopScanning = async () => {
  if (!isScanning) {
    return
  }

  isScanning = false
  qrScanner?.stop()
  videoContainer.style.display = 'none'
  startBtn.style.display = 'block'
  stopBtn.style.display = 'none'
  status.textContent = 'Camera stopped - Click "Start Camera" to scan again'
}

const handleQrScanResult = (result: QrScanner.ScanResult) => {
  const otpUrl = result.data
  status.textContent = 'QR Code found! Processing...'

  // Vibrate if supported
  if (navigator.vibrate) {
    navigator.vibrate(200)
  }

  try {
    const otpConfigs = parseQrPayload(otpUrl)
    if (!otpConfigs) {
      status.textContent = 'Invalid QR code format'
      return
    }

    // Stop scanning and show modal
    stopScanning()
    showConfirmationModal(otpConfigs)
  } catch (err) {
    status.textContent = 'Error parsing QR code'
    console.error(err)
  }
}

let currentOtpConfigs: OtpParameters[] = []

const bindModal = () => {
  modalCloseBtn.addEventListener('click', closeModal)
  modalCancelBtn.addEventListener('click', closeModal)
  modalSaveBtn.addEventListener('click', handleModalSave)

  // Close modal when clicking outside
  modal.addEventListener('click', (e) => {
    if (e.target === modal) {
      closeModal()
    }
  })
}

const showConfirmationModal = (otpConfigs: OtpParameters[]) => {
  currentOtpConfigs = otpConfigs

  if (otpConfigs.length === 1) {
    // Single TOTP QR code
    showSingleTotpModal(otpConfigs[0])
  } else {
    // Google Authenticator export with multiple secrets
    showMultipleTotpModal(otpConfigs)
  }

  modal.classList.add('show')
}

const showSingleTotpModal = (config: OtpParameters) => {
  modalTitle.textContent = 'Confirm TOTP Secret'

  // Build the name from issuer and account name
  const displayName = config.issuer
    ? config.name
      ? `${config.issuer}: ${config.name}`
      : config.issuer
    : config.name || 'Unknown'

  removeAllChildren(modalBody)
  const content = makeNode(
    'div',
    { className: 'modal-secret-item' },
    makeNode('label', { className: 'modal-secret-label' }, 'Name'),
    makeNode('input', {
      type: 'text',
      className: 'modal-secret-input',
      value: displayName,
      id: 'single-secret-name',
    }),
    makeNode('label', { className: 'modal-secret-label' }, 'Secret'),
    makeNode('div', { className: 'modal-secret-value' }, config.secretBase32),
  )

  modalBody.appendChild(content)
  modalSaveBtn.disabled = false
}

const showMultipleTotpModal = (configs: OtpParameters[]) => {
  modalTitle.textContent = 'Select Secrets to Save'
  removeAllChildren(modalBody)

  configs.forEach((config, index) => {
    const displayName = config.issuer
      ? config.name
        ? `${config.issuer}: ${config.name}`
        : config.issuer
      : config.name || 'Unknown'

    const checkbox = makeNode('input', {
      type: 'checkbox',
      className: 'modal-secret-checkbox',
      id: `secret-checkbox-${index}`,
      checked: true,
    }) as HTMLInputElement

    const nameInput = makeNode('input', {
      type: 'text',
      className: 'modal-secret-input',
      value: displayName,
      id: `secret-name-${index}`,
    }) as HTMLInputElement

    const checkboxWrapper = makeNode(
      'div',
      { className: 'modal-secret-checkbox-wrapper' },
      checkbox,
      makeNode('div', { style: 'flex: 1' }, nameInput),
    )

    const item = makeNode(
      'div',
      { className: 'modal-secret-item with-checkbox' },
      checkboxWrapper,
      makeNode('label', { className: 'modal-secret-label' }, 'Secret'),
      makeNode('div', { className: 'modal-secret-value' }, config.secretBase32),
      makeNode(
        'div',
        { className: 'modal-secret-meta' },
        `Issuer: ${config.issuer || 'N/A'} • Account: ${config.name || 'N/A'}`,
      ),
    )

    // Update disabled state based on checkbox
    checkbox.addEventListener('change', () => {
      nameInput.disabled = !checkbox.checked
      updateSaveButtonState()
    })

    modalBody.appendChild(item)
  })

  updateSaveButtonState()
}

const updateSaveButtonState = () => {
  const checkboxes = modalBody.querySelectorAll<HTMLInputElement>(
    '.modal-secret-checkbox',
  )
  const anyChecked = Array.from(checkboxes).some((cb) => cb.checked)
  modalSaveBtn.disabled = !anyChecked
}

const handleModalSave = async () => {
  modalSaveBtn.disabled = true
  modalSaveBtn.textContent = 'Saving...'

  try {
    if (currentOtpConfigs.length === 1) {
      // Single secret
      const nameInput = document.getElementById(
        'single-secret-name',
      ) as HTMLInputElement
      const name = nameInput.value.trim()

      if (!name) {
        alert('Please enter a name')
        return
      }

      await Api.saveSecret({
        name,
        secret: currentOtpConfigs[0].secretBase32,
      })
    } else {
      // Multiple secrets
      const savePromises: Promise<boolean>[] = []

      currentOtpConfigs.forEach((config, index) => {
        const checkbox = document.getElementById(
          `secret-checkbox-${index}`,
        ) as HTMLInputElement

        if (checkbox.checked) {
          const nameInput = document.getElementById(
            `secret-name-${index}`,
          ) as HTMLInputElement
          const name = nameInput.value.trim()

          if (name) {
            savePromises.push(
              Api.saveSecret({
                name,
                secret: config.secretBase32,
              }),
            )
          }
        }
      })

      await Promise.all(savePromises)
    }

    // Show success message
    closeModal()
    secretConfigSuccessCallout.classList.add('show')
    setTimeout(() => {
      secretConfigSuccessCallout.classList.remove('show')
    }, 5000)

    status.textContent = 'Secrets saved successfully!'
  } catch (error) {
    console.error('Error saving secrets:', error)
    status.textContent = 'Error saving secrets. Please try again.'
  } finally {
    modalSaveBtn.disabled = false
    modalSaveBtn.textContent = 'Save'
  }
}

const closeModal = () => {
  modal.classList.remove('show')
  currentOtpConfigs = []
  removeAllChildren(modalBody)
}
