#pragma once

#include "esp32_https_server/HTTPRequest.hpp"
#include "esp32_https_server/HTTPResponse.hpp"

using namespace httpsserver;

class SecretsConfigApi {
 public:
  static void saveSecret(HTTPRequest* req, HTTPResponse* res);
  static void deleteSecret(HTTPRequest* req, HTTPResponse* res);
  static void getSecrets(HTTPRequest* req, HTTPResponse* res);
};
