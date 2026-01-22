import { Api, MaskedSecretDto } from './api'
import {
  modal,
  modalCancelBtn,
  modalCloseBtn,
  modalSaveBtn,
  secretsList,
  secretsManageBtn,
  secretsRefreshBtn,
  secretsUpdateStatus,
} from './bindings'
import { makeNode, removeAllChildren } from './dom-utils'
import { hideModal, showModal } from './modal'

let currentSecretIndexToDelete: number | null = null

export const bindSecretManagementInteractions = () => {
  secretsManageBtn.addEventListener('click', () => {
    // Auto-load secrets when switching to manage mode
    loadSecrets()
  })

  secretsRefreshBtn.addEventListener('click', loadSecrets)

  // Modal handlers for delete confirmation
  modalCloseBtn.addEventListener('click', closeDeleteModal)
  modalCancelBtn.addEventListener('click', closeDeleteModal)
  modal.addEventListener('click', (e: MouseEvent) => {
    if (e.target === modal) {
      closeDeleteModal()
    }
  })
}

const displaySecretsUpdateStatus = (content: string, hideAfterMs?: number) => {
  secretsUpdateStatus.textContent = content
  secretsUpdateStatus.style.display = 'block'
  if (hideAfterMs) {
    setTimeout(() => {
      secretsUpdateStatus.style.display = 'none'
    }, hideAfterMs)
  }
}

const hideSecretsUpdateStatus = () => {
  secretsUpdateStatus.textContent = ''
  secretsUpdateStatus.style.display = 'none'
}

const loadSecrets = async () => {
  removeAllChildren(secretsList)
  displaySecretsUpdateStatus('Loading secrets...')

  try {
    const secrets = await Api.getSecrets()

    if (secrets.length === 0) {
      removeAllChildren(secretsList)
      const emptyMessage = makeNode(
        'div',
        { className: 'secret-list-empty' },
        'No secrets saved yet. Add secrets using Manual Input or Scan.',
      )
      secretsList.appendChild(emptyMessage)
      hideSecretsUpdateStatus()
    } else {
      renderSecretList(secrets)
      displaySecretsUpdateStatus(
        `${secrets.length} secret${secrets.length !== 1 ? 's' : ''} loaded`,
        2000,
      )
    }
  } catch (error) {
    console.error('Error loading secrets:', error)
    displaySecretsUpdateStatus('Error loading secrets. Please try again.')
  }
}

const renderSecretList = (secrets: MaskedSecretDto[]) => {
  removeAllChildren(secretsList)

  secrets.forEach((secret, index) => {
    const deleteBtn = makeNode(
      'button',
      { className: 'secret-list-item-delete' },
      'Delete',
    ) as HTMLButtonElement

    deleteBtn.addEventListener('click', () => {
      showDeleteConfirmation(secret.name, index)
    })

    const item = makeNode(
      'div',
      { className: 'secret-list-item' },
      makeNode('div', { className: 'secret-list-item-name' }, secret.name),
      deleteBtn,
    )

    secretsList.appendChild(item)
  })
}

const showDeleteConfirmation = (secretName: string, index: number) => {
  currentSecretIndexToDelete = index
  showModal({
    title: `Delete Secret ${secretName}?`,
    saveButtonText: 'Delete',
    saveButtonTheme: 'danger',
    saveButtonEventListener: handleDeleteConfirm,
    children: [
      makeNode(
        'div',
        { className: 'modal-confirm' },
        makeNode(
          'div',
          { className: 'modal-confirm-message' },
          'Are you sure you want to delete this secret?',
        ),
        makeNode('div', { className: 'modal-confirm-secret-name' }, secretName),
        makeNode(
          'div',
          { className: 'modal-confirm-warning' },
          'This action cannot be undone.',
        ),
      ),
    ],
  })
}

const handleDeleteConfirm = async () => {
  if (!currentSecretIndexToDelete) return

  modalSaveBtn.disabled = true
  modalSaveBtn.textContent = 'Deleting...'

  try {
    const success = await Api.deleteSecret(currentSecretIndexToDelete)

    if (success) {
      closeDeleteModal()
      displaySecretsUpdateStatus('Secret deleted successfully', 3000)

      // Reload the list
      loadSecrets()
    } else {
      displaySecretsUpdateStatus('Failed to delete secret. Please try again.')
      closeDeleteModal()
    }
  } catch (error) {
    console.error('Error deleting secret:', error)
    displaySecretsUpdateStatus('Error deleting secret. Please try again.')
    closeDeleteModal()
  }
}

const closeDeleteModal = () => {
  hideModal()
  currentSecretIndexToDelete = null
}
