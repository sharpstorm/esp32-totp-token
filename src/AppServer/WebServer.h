#pragma once

#include "Arduino.h"
#include "esp32_https_server/HTTPSServer.hpp"
#include "esp32_https_server/HTTPServer.hpp"
#include "ssl/SslCertProvider.hpp"

using namespace httpsserver;

class WebServer {
 public:
  WebServer();
  void start();
  void stop();
  void loop();
  void registerNode(HTTPNode* node);
  void setDefaultNode(HTTPNode* node);
  void addMiddleware(HTTPSMiddlewareFunction* middleware);

 private:
  HTTPSServer httpsServer;
  HTTPServer httpServer;
};
