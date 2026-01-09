#pragma once

#include <memory>

#include "esp32_https_server/HTTPRequest.hpp"
#include "esp32_https_server/HTTPResponse.hpp"

using namespace httpsserver;
using namespace std;

class HttpUtil {
 public:
  static bool redirectIfNotCaptive(HTTPRequest* req, HTTPResponse* res);

  static unique_ptr<char[]> readRequestBody(HTTPRequest* req, uint16_t bufSize);
};
