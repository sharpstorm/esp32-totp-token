#include "WifiConfigManager.h"

WifiConfigManager::WifiConfigManager() {}

void WifiConfigManager::putConfig(String ssid, String passphrase) {
  preferences.begin(WIFI_CONFIG_NAMESPACE, false);
  preferences.putString(WIFI_CONFIG_SSID_KEY, ssid);
  preferences.putString(WIFI_CONFIG_PASSPHRASE_KEY, passphrase);
  preferences.end();
}

WifiConfig WifiConfigManager::getConfig() {
  preferences.begin(WIFI_CONFIG_NAMESPACE, false);
  String ssid = preferences.getString(WIFI_CONFIG_SSID_KEY, "");
  String passphrase = preferences.getString(WIFI_CONFIG_PASSPHRASE_KEY, "");
  preferences.end();
  bool isValid = ssid.length() > 0;

  return WifiConfig{isValid, ssid, passphrase};
}

WifiConfigManager wifiConfigManager = WifiConfigManager();
