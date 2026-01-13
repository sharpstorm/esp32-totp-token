import { Api } from './api'
import {
  secretConfigInputAddBtn,
  secretConfigInputName,
  secretConfigInputSecret,
  secretConfigSuccessCallout,
} from './bindings'

export const bindManualSecretInput = () => {
  secretConfigInputAddBtn.addEventListener('click', () => {
    const secretName = secretConfigInputName.value
    const secretBase32 = secretConfigInputSecret.value

    if (!secretName || !secretBase32) {
      return
    }

    Api.saveSecret({ name: secretName, secret: secretBase32 }).then(() => {
      console.log('saved')
      secretConfigInputName.value = ''
      secretConfigInputSecret.value = ''
      secretConfigSuccessCallout.classList.add('show')
      setTimeout(() => {
        secretConfigSuccessCallout.classList.remove('show')
      }, 5000)
    })
  })
}
