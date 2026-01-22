import { Api, WifiNetwork } from './api'
import {
  currentSsidDiv,
  networksListContainer,
  passphraseContainer,
  passphraseInput,
  saveBtn,
  scanBtn,
  selectedSsidDiv,
  wifiSaveSuccessCallout,
} from './bindings'
import { makeNode, removeAllChildren } from './dom-utils'

let currentSsid: string | null = null
let selectedSsid: string | null = null

export const bindWifiConfigInteractions = () => {
  // Load current WiFi on page load
  loadCurrentWifi()

  // Bind scan button
  scanBtn.addEventListener('click', async () => {
    scanBtn.disabled = true
    scanBtn.textContent = 'Scanning...'
    removeAllChildren(networksListContainer)

    try {
      const networks = await Api.scanWifiNetworks()
      displayNetworks(networks)
    } catch (error) {
      console.error('Failed to scan networks:', error)
      networksListContainer.appendChild(
        makeNode(
          'div',
          { className: 'wifi-error' },
          'Failed to scan networks. Please try again.',
        ),
      )
    } finally {
      scanBtn.disabled = false
      scanBtn.textContent = 'Scan for Networks'
    }
  })

  // Bind save button
  saveBtn.addEventListener('click', async () => {
    if (!selectedSsid || !passphraseInput.value) {
      return
    }

    saveBtn.disabled = true
    saveBtn.textContent = 'Saving...'

    try {
      const success = await Api.saveWifi({
        ssid: selectedSsid,
        passphrase: passphraseInput.value,
      })

      if (success) {
        currentSsid = selectedSsid
        currentSsidDiv.textContent = currentSsid
        passphraseInput.value = ''
        passphraseContainer.style.display = 'none'
        selectedSsid = null

        // Show success message
        wifiSaveSuccessCallout.classList.add('show')
        setTimeout(() => {
          wifiSaveSuccessCallout.classList.remove('show')
        }, 3000)
      } else {
        alert('Failed to save WiFi configuration')
      }
    } catch (error) {
      console.error('Failed to save WiFi:', error)
      alert('Failed to save WiFi configuration')
    } finally {
      saveBtn.disabled = false
      saveBtn.textContent = 'Save WiFi Configuration'
    }
  })

  async function loadCurrentWifi() {
    try {
      currentSsid = await Api.getCurrentWifi()
      currentSsidDiv.textContent = currentSsid || 'Not configured'
      currentSsidDiv.className = currentSsid
        ? 'wifi-current-ssid'
        : 'wifi-current-ssid wifi-not-configured'
    } catch (error) {
      console.error('Failed to load current WiFi:', error)
      currentSsidDiv.textContent = 'Error loading'
      currentSsidDiv.className = 'wifi-current-ssid wifi-not-configured'
    }
  }

  function displayNetworks(networks: WifiNetwork[]) {
    if (networks.length === 0) {
      networksListContainer.appendChild(
        makeNode('div', { className: 'wifi-no-networks' }, 'No networks found'),
      )
      return
    }

    removeAllChildren(networksListContainer)

    networks.forEach((network) => {
      const networkItem = makeNode(
        'div',
        { className: 'wifi-network-item' },
        makeNode(
          'div',
          { className: 'wifi-network-info' },
          makeNode('div', { className: 'wifi-network-ssid' }, network.ssid),
          makeNode(
            'div',
            { className: 'wifi-network-details' },
            `${network.security} • Signal: ${network.rssi} dBm`,
          ),
        ),
      )

      networkItem.addEventListener('click', () => {
        selectNetwork(network.ssid)
      })

      networksListContainer.appendChild(networkItem)
    })
  }

  function selectNetwork(ssid: string) {
    // If there's a current SSID and it's different from the selected one, confirm
    if (currentSsid && currentSsid !== ssid) {
      const confirmed = confirm(
        `You currently have "${currentSsid}" configured. Do you want to replace it with "${ssid}"?`,
      )
      if (!confirmed) {
        return
      }
    }

    selectedSsid = ssid
    selectedSsidDiv.textContent = ssid
    passphraseInput.value = ''
    passphraseContainer.style.display = 'block'

    // Highlight selected network
    const allNetworkItems =
      networksListContainer.getElementsByClassName('wifi-network-item')
    for (const item of allNetworkItems) {
      const ssidLabel = item.getElementsByClassName('wifi-network-ssid')[0]
      if (ssidLabel?.textContent === ssid) {
        item.classList.add('wifi-network-item-selected')
      } else {
        item.classList.remove('wifi-network-item-selected')
      }
    }
  }
}
