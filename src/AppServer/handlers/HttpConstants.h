#pragma once

class MimeType {
 public:
  static const char* JSON;
  static const char* Plain;
};

class HttpHeader {
 public:
  static const char* ContentType;
  static const char* ContentLength;
  static const char* Host;
  static const char* Location;
};
