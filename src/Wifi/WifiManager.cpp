#include "WifiManager.h"

#include "../Util/Base32.h"

WifiManager::WifiManager() {
  // Compute hotspot SSID
  uint64_t espChipId = ESP.getEfuseMac();

  byte deviceId[] = {
      espChipId & 0xFF,         (espChipId >> 8) & 0xFF,
      (espChipId >> 16) & 0xFF, (espChipId >> 24) & 0xFF,
      (espChipId >> 32) & 0xFF,
  };
  byte* deviceIdString;
  Base32::toBase32(deviceId, 64, deviceIdString);

  char basePrefix[] = HOTSPOT_SSID_PREFIX;
  strcpy(basePrefix, hotspotName);
  strcpy((char*)deviceIdString, &(hotspotName[5]));
  hotspotName[13] = 0;

  free(deviceIdString);
}

String WifiManager::getConfiguredSsid() { return WIFI_SSID; }

bool WifiManager::connectToWifi() {
  uint64_t timeSinceConnection = millis();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

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
  WiFi.softAP(hotspotName, "hellohello");
}

void WifiManager::stopWifiHotspot() {
  WiFi.softAPdisconnect();
  WiFi.setSleep(true);
  WiFi.mode(WIFI_OFF);
}

WifiManager wifiManager = WifiManager();
