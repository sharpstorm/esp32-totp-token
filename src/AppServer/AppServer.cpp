#include "AppServer.h"

#include "handlers/AppHandlers.h"
#include "handlers/HttpUtil.h"
#include "handlers/WifiConfigApi.h"

AppServer::AppServer()
    : webServer(),
      routeRoot("/", "GET", &AppHandlers::handleRoot),
      routeIndexCss("/index.css", "GET", &AppHandlers::handleCss),
      routeIndexJs("/index.js", "GET", &AppHandlers::handleJs),
      routeScannerJs("/qr-scanner-worker.min.js", "GET",
                     &AppHandlers::handleScannerJs),
      route404("", "GET", &AppHandlers::handle404),

      wifiApiScan("/wifi/scan", "POST", &WifiConfigApi::scanWifiNetworks),
      wifiApiSaveConfig("/wifi/config", "PUT", &WifiConfigApi::saveWifiNetwork),
      wifiApiGetConfig("/wifi/config", "GET",
                       &WifiConfigApi::getSavedWifiNetwork) {
  webServer.addMiddleware(&AppServer::captiveRedirectMiddleware);
  // Register routes
  webServer.registerNode(&routeRoot);
  webServer.registerNode(&routeIndexCss);
  webServer.registerNode(&routeIndexJs);
  webServer.registerNode(&routeScannerJs);

  webServer.registerNode(&route404);
  webServer.registerNode(&wifiApiScan);
  webServer.registerNode(&wifiApiSaveConfig);
  webServer.registerNode(&wifiApiGetConfig);
}

void AppServer::start() { webServer.start(); }

void AppServer::stop() { webServer.stop(); }

void AppServer::loop() { webServer.loop(); }

void AppServer::captiveRedirectMiddleware(httpsserver::HTTPRequest* req,
                                          httpsserver::HTTPResponse* res,
                                          OnReceiveCb next) {
  if (HttpUtil::redirectIfNotCaptive(req, res)) {
    return;
  }

  next();
}
