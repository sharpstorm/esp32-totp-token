#pragma once

#include <SSLCert.hpp>

#include "SslCertStore.h"

using namespace httpsserver;

class SslCertProvider {
 public:
  static SSLCert cert;
};
