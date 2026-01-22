import { bindContentTabInteraction } from './tabs'
import { bindSecretInputInteractions } from './secret-input'
import { bindWifiConfigInteractions } from './wifi-config'
import { bindSecretManagementInteractions } from './secret-management'
import { bindTimeSyncInteractions } from './time-sync'

document.addEventListener('DOMContentLoaded', () => {
  bindContentTabInteraction()
  bindSecretInputInteractions()
  bindSecretManagementInteractions()
  bindTimeSyncInteractions()
  bindWifiConfigInteractions()
})
