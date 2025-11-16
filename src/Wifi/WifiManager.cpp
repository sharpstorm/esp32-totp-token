#include "WifiManager.h"

#include "../Util/Base32.h"
#include "WifiConfigManager.h"

WifiManager::WifiManager() {
  // Compute hotspot SSID
  uint64_t espChipId = ESP.getEfuseMac();

  byte deviceId[] = {
      (byte)(espChipId & 0xFF),         (byte)((espChipId >> 8) & 0xFF),
      (byte)((espChipId >> 16) & 0xFF), (byte)((espChipId >> 24) & 0xFF),
      (byte)((espChipId >> 32) & 0xFF),
  };
  byte* deviceIdString;
  Base32::toBase32(deviceId, 64, deviceIdString);

  char basePrefix[] = HOTSPOT_SSID_PREFIX;
  strcpy(hotspotName, basePrefix);
  strcpy(&(hotspotName[5]), (char*)deviceIdString);
  hotspotName[13] = 0;

  free(deviceIdString);
  strcpy(hotspotPassword, HOTSPOT_SSID_PASSPHRASE);
}

String WifiManager::getConfiguredSsid() {
  WifiConfig config = wifiConfigManager.getConfig();
  if (!config.isConfigured) {
    return "Unconfigured WiFi";
  }
  return config.ssid;
}

bool WifiManager::connectToWifi() {
  uint64_t timeSinceConnection = millis();
  WifiConfig config = wifiConfigManager.getConfig();
  if (!config.isConfigured) {
    return false;
  }

  WiFi.begin(config.ssid, config.passphrase);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (millis() - timeSinceConnection > 10000) {
      return false;
    }
  }

  return true;
}

void WifiManager::disconnectWifi() {
  WiFi.disconnect(true);
  WiFi.setSleep(true);
  WiFi.mode(WIFI_OFF);
}

void WifiManager::startWifiHotspot() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(hotspotName, hotspotPassword);
}

void WifiManager::stopWifiHotspot() {
  WiFi.softAPdisconnect();
  WiFi.setSleep(true);
  WiFi.mode(WIFI_OFF);
}

String WifiManager::getConfiguredHotspotSsid() { return hotspotName; }
String WifiManager::getConfiguredHotspotPassword() { return hotspotPassword; }
IPAddress WifiManager::getHotspotGatewayIp() { return WiFi.softAPIP(); }

WifiManager wifiManager = WifiManager();
