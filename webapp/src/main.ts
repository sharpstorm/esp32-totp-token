import { bindContentTabInteraction } from './tabs'
import { bindSecretInputInteractions } from './secret-input'
import { bindWifiConfigInteractions } from './wifi-config'
import { bindSecretManagementInteractions } from './secret-management'

document.addEventListener('DOMContentLoaded', () => {
  bindContentTabInteraction()
  bindSecretInputInteractions()
  bindSecretManagementInteractions()
  bindWifiConfigInteractions()
})
