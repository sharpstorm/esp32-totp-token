#pragma once

#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <memory>

using namespace httpsserver;
using namespace std;

class HttpUtil {
 public:
  static bool redirectIfNotCaptive(HTTPRequest* req, HTTPResponse* res);

  static unique_ptr<char[]> readRequestBody(HTTPRequest* req, uint16_t bufSize);
};
