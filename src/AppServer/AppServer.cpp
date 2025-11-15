#include "AppServer.h"

AppServer::AppServer()
    : webServer(WebServer()),
      nodeRoot(ResourceNode("/", "GET", &AppHandlers::handleRoot)),
      node404(ResourceNode("", "GET", &AppHandlers::handle404)) {
  // Register routes
  webServer.registerNode(&nodeRoot);
  webServer.registerNode(&node404);
}

void AppServer::start() { webServer.start(); }

void AppServer::stop() { webServer.stop(); }

void AppServer::loop() { webServer.loop(); }