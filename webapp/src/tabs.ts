import { ContentTabId, appContentContainerState } from './app-state'
import {
  contentPaneSecretConfig,
  contentPaneTimeSync,
  contentPaneWifiConfig,
  tabSecretConfig,
  tabTimeSync,
  tabWifiConfig,
} from './bindings'
import { removeClassFromAllElements } from './dom-utils'

type TabDefinition = {
  tabButton: HTMLButtonElement
  tabId: ContentTabId
  contentPane: HTMLDivElement
}

const tabDefinitions: TabDefinition[] = [
  {
    tabButton: tabWifiConfig,
    tabId: 'wifiConfig',
    contentPane: contentPaneWifiConfig,
  },
  {
    tabButton: tabSecretConfig,
    tabId: 'secretConfig',
    contentPane: contentPaneSecretConfig,
  },
  {
    tabButton: tabTimeSync,
    tabId: 'timeSync',
    contentPane: contentPaneTimeSync,
  },
]

export const bindContentTabInteraction = () => {
  tabDefinitions.forEach(({ tabButton, tabId, contentPane }) => {
    tabButton.addEventListener('click', () => {
      if (appContentContainerState.activeTab === tabId) {
        return
      }

      removeClassFromAllElements('tab-active')
      removeClassFromAllElements('content-pane-active')
      tabButton.classList.add('tab-active')
      contentPane.classList.add('content-pane-active')
      appContentContainerState.activeTab = tabId
    })
  })
}
