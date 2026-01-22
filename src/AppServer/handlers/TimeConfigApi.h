#pragma once

#include "esp32_https_server/HTTPRequest.hpp"
#include "esp32_https_server/HTTPResponse.hpp"

using namespace httpsserver;

class TimeConfigApi {
 public:
  static void setTime(HTTPRequest* req, HTTPResponse* res);
  static void getTime(HTTPRequest* req, HTTPResponse* res);
};
