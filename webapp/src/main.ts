import { bindContentTabInteraction } from './tabs'
import { bindSecretInputInteractions } from './secret-input'

document.addEventListener('DOMContentLoaded', () => {
  bindContentTabInteraction()
  bindSecretInputInteractions()
})
