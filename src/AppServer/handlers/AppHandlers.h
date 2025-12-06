#pragma once

#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>

using namespace httpsserver;

class AppHandlers {
 public:
  static void handleRoot(HTTPRequest* req, HTTPResponse* res);
  static void handleJs(HTTPRequest* req, HTTPResponse* res);
  static void handleScannerJs(HTTPRequest* req, HTTPResponse* res);
  static void handleCss(HTTPRequest* req, HTTPResponse* res);
  static void handle404(HTTPRequest* req, HTTPResponse* res);
};
