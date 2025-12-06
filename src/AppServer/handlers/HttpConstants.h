#pragma once

class MimeType {
 public:
  static const char* JSON;
  static const char* Plain;
  static const char* Html;
  static const char* Css;
  static const char* Javascript;
};

class HttpHeader {
 public:
  static const char* ContentType;
  static const char* ContentLength;
  static const char* Host;
  static const char* Location;
  static const char* Connection;
};
