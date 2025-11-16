#pragma once

#include "Preferences.h"

#define WIFI_CONFIG_NAMESPACE "wifi-config"

#define WIFI_CONFIG_SSID_KEY "wifi-ssid"
#define WIFI_CONFIG_PASSPHRASE_KEY "passphrase"

struct WifiConfig {
  boolean isConfigured;
  String ssid;
  String passphrase;
};

class WifiConfigManager {
 private:
  Preferences preferences;

 public:
  WifiConfigManager();

  void putConfig(String ssid, String passphrase);
  WifiConfig getConfig();
};

extern WifiConfigManager wifiConfigManager;
