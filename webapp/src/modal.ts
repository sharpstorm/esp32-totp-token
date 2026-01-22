import { modal, modalBody, modalSaveBtn, modalTitle } from './bindings'
import { removeAllChildren } from './dom-utils'
import { AddPrefixToObject } from './types'

const SaveButtonThemes = {
  default: 'btn btn-primary',
  danger: 'btn btn-danger',
}

export type ModalSaveButtonArgs = {
  theme?: keyof typeof SaveButtonThemes
  text?: string
  eventListener?: () => void
  disabled?: boolean
}

export type ShowModalArgs<T extends Node> = {
  title?: string
  children?: T[]
} & AddPrefixToObject<ModalSaveButtonArgs, 'saveButton'>

export const DEFAULT_MODAL_SAVE_TEXT = 'Save'

export const showModal = <T extends Node>({
  title = '',
  children = [],
  saveButtonText,
  saveButtonTheme,
  saveButtonEventListener,
  saveButtonDisabled,
}: ShowModalArgs<T>) => {
  modalTitle.textContent = title
  removeAllChildren(modalBody)

  children.forEach((child) => {
    modalBody.appendChild(child)
  })

  // Change save button to delete button
  setModalSaveButton({
    text: saveButtonText,
    disabled: saveButtonDisabled,
    eventListener: saveButtonEventListener,
    theme: saveButtonTheme,
  })

  modal.classList.add('show')
}

export const setModalSaveButton = ({
  theme = 'default',
  text = 'Save',
  eventListener = () => {},
  disabled = false,
}: ModalSaveButtonArgs) => {
  modalSaveBtn.textContent = text
  modalSaveBtn.className = SaveButtonThemes[theme]
  modalSaveBtn.onclick = eventListener
  modalSaveBtn.disabled = disabled
}

export const hideModal = () => {
  modalTitle.textContent = ''
  removeAllChildren(modalBody)

  // Change save button to delete button
  modalSaveBtn.textContent = DEFAULT_MODAL_SAVE_TEXT
  modalSaveBtn.className = SaveButtonThemes['default']
  modalSaveBtn.onclick = () => {}
  modalSaveBtn.disabled = false

  modal.classList.remove('show')
}
