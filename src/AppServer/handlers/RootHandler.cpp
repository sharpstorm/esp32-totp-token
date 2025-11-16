#include "AppHandlers.h"

void AppHandlers::handleRoot(HTTPRequest* req, HTTPResponse* res) {
  res->setHeader("Content-Type", "text/html");

  res->println("<!DOCTYPE html>");
  res->println("<html>");
  res->println("<head><title>Hello World!</title></head>");
  res->println("<body>");
  res->println("<h1>Hello World!</h1>");

  res->print("<p>Your server is running for ");
  res->print((int)(millis() / 1000), DEC);
  res->println(" seconds.</p>");

  // You can check if you are connected over a secure connection, eg. if you
  // want to use authentication and redirect the user to a secure connection
  // for that
  if (req->isSecure()) {
    res->println("<p>You are connected via <strong>HTTPS</strong>.</p>");
  } else {
    res->println("<p>You are connected via <strong>HTTP</strong>.</p>");
  }

  res->println("</body>");
  res->println("</html>");
}
