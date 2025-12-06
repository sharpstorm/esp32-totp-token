#pragma once

#include "WebServer.h"

using namespace httpsserver;

class AppServer {
 public:
  AppServer();
  void start();
  void stop();
  void loop();

  static void captiveRedirectMiddleware(httpsserver::HTTPRequest* req,
                                        httpsserver::HTTPResponse* res,
                                        OnReceiveCb next);

 private:
  WebServer webServer;

  ResourceNode routeRoot;
  ResourceNode routeIndexJs;
  ResourceNode routeScannerJs;
  ResourceNode routeIndexCss;

  ResourceNode route404;
  ResourceNode wifiApiScan;
  ResourceNode wifiApiSaveConfig;
  ResourceNode wifiApiGetConfig;
};
