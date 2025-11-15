#include "AppHandlers.hpp"

void AppHandlers::handle404(HTTPRequest* req, HTTPResponse* res) {
  req->discardRequestBody();
  res->setStatusCode(404);
  res->setStatusText("Not Found");
  res->setHeader("Content-Type", "text/html");
  res->println("<!DOCTYPE html>");
  res->println("<html>");
  res->println("<head><title>Not Found</title></head>");
  res->println(
      "<body><h1>404 Not Found</h1><p>The requested resource was not found on "
      "this server.</p></body>");
  res->println("</html>");
}
