#pragma once

#include <HTTPSServer.hpp>
#include <HTTPServer.hpp>

#include "Arduino.h"
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
