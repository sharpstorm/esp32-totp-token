#pragma once

#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>

using namespace httpsserver;

class WifiConfigApi {
 public:
  static void scanWifiNetworks(HTTPRequest* req, HTTPResponse* res);
  static void saveWifiNetwork(HTTPRequest* req, HTTPResponse* res);
  static void getSavedWifiNetwork(HTTPRequest* req, HTTPResponse* res);
};
