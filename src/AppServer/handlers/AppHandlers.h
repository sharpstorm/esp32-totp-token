#pragma once

#include "esp32_https_server/HTTPRequest.hpp"
#include "esp32_https_server/HTTPResponse.hpp"

using namespace httpsserver;

class AppHandlers {
 public:
  static void handleRoot(HTTPRequest* req, HTTPResponse* res);
  static void handleJs(HTTPRequest* req, HTTPResponse* res);
  static void handleScannerJs(HTTPRequest* req, HTTPResponse* res);
  static void handleCss(HTTPRequest* req, HTTPResponse* res);
  static void handle404(HTTPRequest* req, HTTPResponse* res);
};
