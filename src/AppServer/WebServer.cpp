#include "WebServer.h"

WebServer::WebServer() : httpServer(), httpsServer(&SslCertProvider::cert) {}

void WebServer::start() {
  httpServer.start();
  httpsServer.start();
}

void WebServer::stop() {
  httpServer.stop();
  httpsServer.stop();
}

void WebServer::loop() {
  httpServer.loop();
  httpsServer.loop();
}

void WebServer::registerNode(HTTPNode* node) {
  httpServer.registerNode(node);
  httpsServer.registerNode(node);
}

void WebServer::setDefaultNode(HTTPNode* node) {
  httpServer.setDefaultNode(node);
  httpsServer.setDefaultNode(node);
}

void WebServer::addMiddleware(HTTPSMiddlewareFunction* middleware) {
  httpServer.addMiddleware(middleware);
  httpsServer.addMiddleware(middleware);
}
