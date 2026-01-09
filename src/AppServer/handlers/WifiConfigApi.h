#pragma once

#include "esp32_https_server/HTTPRequest.hpp"
#include "esp32_https_server/HTTPResponse.hpp"

using namespace httpsserver;

class WifiConfigApi {
 public:
  static void scanWifiNetworks(HTTPRequest* req, HTTPResponse* res);
  static void saveWifiNetwork(HTTPRequest* req, HTTPResponse* res);
  static void getSavedWifiNetwork(HTTPRequest* req, HTTPResponse* res);
};
