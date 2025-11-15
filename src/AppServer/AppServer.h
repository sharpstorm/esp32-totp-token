#pragma once

#include "WebServer.h"
#include "handlers/AppHandlers.hpp"

using namespace httpsserver;

class AppServer {
 public:
  AppServer();
  void start();
  void stop();
  void loop();

 private:
  WebServer webServer;

  ResourceNode nodeRoot;
  ResourceNode node404;
};
