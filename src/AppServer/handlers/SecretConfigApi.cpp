#include <Util/Base32.h>

#include "../../Secret/SecretManager.h"
#include "ArduinoJson.h"
#include "HttpConstants.h"
#include "HttpUtil.h"
#include "SecretsConfigApi.h"

using namespace std;

void SecretsConfigApi::getSecrets(HTTPRequest* req, HTTPResponse* res) {
  secretManager.start();
  int secretCount = secretManager.getSecretCount();

  JsonDocument respBody;
  JsonArray networks = respBody["secrets"].to<JsonArray>();
  respBody["count"] = secretCount;

  for (int i = 0; i < secretCount; i++) {
    Secret secret = secretManager.readRecord(i);
    JsonObject secretRecord = networks.add<JsonObject>();
    secretRecord["name"] = secret.getName();
  }

  res->setStatusCode(200);
  res->setHeader(HttpHeader::ContentType, MimeType::JSON);
  serializeJson(respBody, *res);
}

void SecretsConfigApi::saveSecret(HTTPRequest* req, HTTPResponse* res) {
  unique_ptr<char[]> reqBodyText = HttpUtil::readRequestBody(req, 200);

  if (reqBodyText == nullptr) {
    res->setStatusCode(413);
    res->println("Request entity too large");
    return;
  }

  JsonDocument reqBody;
  JsonDocument filter;
  filter["name"] = true;
  filter["secret"] = true;
  deserializeJson(reqBody, reqBodyText.get(),
                  DeserializationOption::Filter(filter));

  if (reqBody["name"].isUnbound() || reqBody["secret"].isUnbound()) {
    res->setStatusCode(400);
    res->setHeader(HttpHeader::ContentType, MimeType::JSON);
    res->println("{ \"success\": false }");
    return;
  }

  String secretBase32 = reqBody["secret"].as<String>();
  String secretName = reqBody["name"].as<String>();

  byte* secretBytes;
  Base32::fromBase32((byte*)secretBase32.c_str(), secretBase32.length(),
                     secretBytes);
  Secret secret(secretBase32.length() * 5);
  memcpy(secret.get(), secretBytes, secret.byteLen());
  free(secretBytes);

  secret.setName(secretName);
  secretManager.start();
  secretManager.putRecord(&secret);

  res->setStatusCode(200);
  res->setHeader(HttpHeader::ContentType, MimeType::JSON);
  res->println("{ \"success\": true }");
}

void SecretsConfigApi::deleteSecret(HTTPRequest* req, HTTPResponse* res) {
  // Get name from query parameter
  ResourceParameters* params = req->getParams();
  string indexParam;
  params->getQueryParameter("index", indexParam);

  if (indexParam.empty()) {
    res->setStatusCode(400);
    res->setHeader(HttpHeader::ContentType, MimeType::JSON);
    res->println(
        "{ \"success\": false, \"error\": \"Missing index parameter\" }");
    return;
  }

  int indexToDelete = stoi(indexParam);

  // Find the secret by name
  secretManager.start();
  int secretCount = secretManager.getSecretCount();
  if (indexToDelete < 0 || indexToDelete >= secretCount) {
    res->println("{ \"success\": false, \"error\": \"Invalid Index\" }");
    return;
  }

  secretManager.deleteRecord(indexToDelete);

  res->setStatusCode(200);
  res->setHeader(HttpHeader::ContentType, MimeType::JSON);
  res->println("{ \"success\": true }");
}
