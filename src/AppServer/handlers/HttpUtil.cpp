#include "HttpUtil.h"

#include "../../Wifi/WifiManager.h"
#include "HttpConstants.h"

bool HttpUtil::redirectIfNotCaptive(HTTPRequest* req, HTTPResponse* res) {
  const String myIp = wifiManager.getHotspotGatewayIp().toString();
  if (req->getHeader(HttpHeader::Host) == myIp.c_str()) {
    return false;
  }

  String protocol = req->isSecure() ? "https://" : "http://";
  res->setHeader(HttpHeader::Location, (protocol + myIp).c_str());
  res->setHeader(HttpHeader::ContentType, MimeType::Plain);
  res->setStatusCode(302);
  res->finalize();

  return true;
}

unique_ptr<char[]> HttpUtil::readRequestBody(HTTPRequest* req,
                                             uint16_t bufSize) {
  unique_ptr<char[]> bufferPtr(new char[bufSize]);
  char* buffer = bufferPtr.get();
  memset(buffer, 0, bufSize);
  size_t idx = 0;
  while (!req->requestComplete() && idx < bufSize) {
    idx += req->readChars(buffer + idx, bufSize - idx);
  }

  if (!req->requestComplete()) {
    return nullptr;
  }

  return move(bufferPtr);
}
