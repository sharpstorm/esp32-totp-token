#include "CaptiveDnsServer.h"

#include "../Wifi/WifiManager.h"

CaptiveDnsServer::CaptiveDnsServer() {
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
}

void CaptiveDnsServer::start() {
  dnsServer.start(DNS_PORT, "*", wifiManager.getHotspotGatewayIp());
}

void CaptiveDnsServer::stop() { dnsServer.stop(); }

void CaptiveDnsServer::loop() { dnsServer.processNextRequest(); }
