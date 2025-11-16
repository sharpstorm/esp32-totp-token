#pragma once

#include "DNSServer.h"

#define DNS_PORT 53

class CaptiveDnsServer {
 public:
  CaptiveDnsServer();

  void start();
  void stop();
  void loop();

 private:
  DNSServer dnsServer;
};
