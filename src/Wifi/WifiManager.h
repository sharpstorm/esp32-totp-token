#pragma once

#include "Arduino.h"
#include "Config.h"
#include "WiFi.h"

class WifiManager {
 public:
  WifiManager();

  // As station
  bool connectToWifi();
  void disconnectWifi();
  String getConfiguredSsid();

  // As AP
  void startWifiHotspot();
  void stopWifiHotspot();
  String getConfiguredHotspotSsid();
  String getConfiguredHotspotPassword();
  IPAddress getHotspotGatewayIp();

 private:
  char hotspotName[14];
  char hotspotPassword[16];
};

extern WifiManager wifiManager;
