import { bindContentTabInteraction } from './tabs'
import { bindSecretInputInteractions } from './secret-input'
import { bindWifiConfigInteractions } from './wifi-config'

document.addEventListener('DOMContentLoaded', () => {
  bindContentTabInteraction()
  bindSecretInputInteractions()
  bindWifiConfigInteractions()
})
