#pragma once

#include "SslCertStore.h"
#include "esp32_https_server/SSLCert.hpp"

using namespace httpsserver;

class SslCertProvider {
 public:
  static SSLCert cert;
};
