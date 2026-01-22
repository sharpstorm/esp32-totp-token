import { Api } from './api'
import {
  tabTimeSync,
  timeBrowserDate,
  timeBrowserDisplay,
  timeCurrentDate,
  timeCurrentDisplay,
  timeSyncBtn,
  timeSyncRefreshBtn,
  timeSyncSuccessCallout,
} from './bindings'

let browserTimeInterval: number | null = null

export const bindTimeSyncInteractions = () => {
  tabTimeSync.addEventListener('click', () => {
    // Load device time and start browser time updates
    loadDeviceTime()
    startBrowserTimeUpdates()
  })

  // Other tabs - stop browser time updates
  document.querySelectorAll('#tab-container .tab').forEach((tab) => {
    if (tab !== tabTimeSync) {
      tab.addEventListener('click', stopBrowserTimeUpdates)
    }
  })

  timeSyncRefreshBtn.addEventListener('click', loadDeviceTime)
  timeSyncBtn.addEventListener('click', syncTimeFromBrowser)
}

const loadDeviceTime = async () => {
  try {
    const deviceTime = await Api.getTime()

    if (deviceTime) {
      const timeStr = formatTime(
        deviceTime.hour,
        deviceTime.minute,
        deviceTime.second,
      )
      const dateStr = formatDate(
        deviceTime.year,
        deviceTime.month,
        deviceTime.day,
      )

      timeCurrentDisplay.textContent = timeStr
      timeCurrentDate.textContent = dateStr
    } else {
      timeCurrentDisplay.textContent = 'Error'
      timeCurrentDate.textContent = 'Unable to load'
    }
  } catch (error) {
    console.error('Error loading device time:', error)
    timeCurrentDisplay.textContent = 'Error'
    timeCurrentDate.textContent = 'Unable to load'
  }
}

const startBrowserTimeUpdates = () => {
  // Update immediately
  updateBrowserTime()

  // Update every second
  browserTimeInterval = window.setInterval(updateBrowserTime, 1000)
}

const stopBrowserTimeUpdates = () => {
  if (browserTimeInterval !== null) {
    clearInterval(browserTimeInterval)
    browserTimeInterval = null
  }
}

const updateBrowserTime = () => {
  const now = new Date()
  const timeStr = formatTime(now.getHours(), now.getMinutes(), now.getSeconds())
  const dateStr = formatDate(
    now.getFullYear(),
    now.getMonth() + 1,
    now.getDate(),
  )

  timeBrowserDisplay.textContent = timeStr
  timeBrowserDate.textContent = dateStr
}

const formatTime = (hour: number, minute: number, second: number): string => {
  return `${pad(hour)}:${pad(minute)}:${pad(second)}`
}

const formatDate = (year: number, month: number, day: number): string => {
  return `${year}/${pad(month)}/${pad(day)}`
}

const pad = (num: number): string => {
  return num.toString().padStart(2, '0')
}

const syncTimeFromBrowser = async () => {
  timeSyncBtn.disabled = true
  timeSyncBtn.textContent = 'Syncing...'

  try {
    // Get current Unix timestamp in seconds
    const timestamp =
      Math.floor(Date.now() / 1000) - new Date().getTimezoneOffset() * 60

    const success = await Api.setTime(timestamp)

    if (success) {
      // Show success message
      timeSyncSuccessCallout.classList.add('show')
      setTimeout(() => {
        timeSyncSuccessCallout.classList.remove('show')
      }, 5000)

      // Reload device time to show updated value
      setTimeout(() => {
        loadDeviceTime()
      }, 500)
    } else {
      alert('Failed to sync time. Please try again.')
    }
  } catch (error) {
    console.error('Error syncing time:', error)
    alert('Error syncing time. Please try again.')
  } finally {
    timeSyncBtn.disabled = false
    timeSyncBtn.textContent = 'Sync Time from Browser'
  }
}
