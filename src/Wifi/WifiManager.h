#pragma once

#include "Arduino.h"
#include "Config.h"
#include "WiFi.h"

class WifiManager {
 public:
  WifiManager();

  String getConfiguredSsid();

  bool connectToWifi();
  void disconnectWifi();

  void startWifiHotspot();
  void stopWifiHotspot();

 private:
  char hotspotName[14];
};

extern WifiManager wifiManager;
