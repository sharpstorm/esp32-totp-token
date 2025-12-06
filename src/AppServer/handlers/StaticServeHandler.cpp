#include "AppHandlers.h"
#include "HttpConstants.h"

extern const uint8_t indexHtmlFileStart[] asm(
    "_binary_webapp_dist_index_html_start");
extern const uint8_t indexHtmlFileEnd[] asm(
    "_binary_webapp_dist_index_html_end");
extern const uint8_t indexCssFileStart[] asm(
    "_binary_webapp_dist_index_css_start");
extern const uint8_t indexCssFileEnd[] asm("_binary_webapp_dist_index_css_end");
extern const uint8_t indexJsFileStart[] asm(
    "_binary_webapp_dist_index_js_start");
extern const uint8_t indexJsFileEnd[] asm("_binary_webapp_dist_index_js_end");
extern const uint8_t scannerJsFileStart[] asm(
    "_binary_webapp_dist_qr_scanner_worker_min_js_start");
extern const uint8_t scannerJsFileEnd[] asm(
    "_binary_webapp_dist_qr_scanner_worker_min_js_end");

void AppHandlers::handleRoot(HTTPRequest* req, HTTPResponse* res) {
  res->setHeader(HttpHeader::ContentType, MimeType::Html);
  res->setHeader(HttpHeader::Connection, "keep-alive");
  size_t dataLen = indexHtmlFileEnd - indexHtmlFileStart - 1;
  res->write(indexHtmlFileStart, dataLen);
}

void AppHandlers::handleCss(HTTPRequest* req, HTTPResponse* res) {
  res->setHeader(HttpHeader::ContentType, MimeType::Css);
  res->setHeader(HttpHeader::Connection, "keep-alive");
  size_t dataLen = indexCssFileEnd - indexCssFileStart - 1;
  res->write(indexCssFileStart, dataLen);
}

void AppHandlers::handleJs(HTTPRequest* req, HTTPResponse* res) {
  res->setHeader(HttpHeader::ContentType, MimeType::Javascript);
  res->setHeader(HttpHeader::Connection, "keep-alive");
  size_t dataLen = indexJsFileEnd - indexJsFileStart - 1;
  res->write(indexJsFileStart, dataLen);
}

void AppHandlers::handleScannerJs(HTTPRequest* req, HTTPResponse* res) {
  res->setHeader(HttpHeader::ContentType, MimeType::Javascript);
  res->setHeader(HttpHeader::Connection, "keep-alive");
  size_t dataLen = scannerJsFileEnd - scannerJsFileStart - 1;
  res->write(scannerJsFileStart, dataLen);
}
