#include "WifiConfigApi.h"

#include "../../Wifi/WifiConfigManager.h"
#include "../../Wifi/WifiManager.h"
#include "ArduinoJson.h"
#include "HttpConstants.h"
#include "HttpUtil.h"

using namespace std;

String wifiEncryptionTypeToString(wifi_auth_mode_t authMode) {
  switch (authMode) {
    case WIFI_AUTH_OPEN:
      return "open";
    case WIFI_AUTH_WEP:
      return "WEP";
    case WIFI_AUTH_WPA_PSK:
      return "WPA";
    case WIFI_AUTH_WPA2_PSK:
      return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:
      return "WPA+WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE:
      return "WPA2-EAP";
    case WIFI_AUTH_WPA3_PSK:
      return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:
      return "WPA2+WPA3";
    case WIFI_AUTH_WAPI_PSK:
      return "WAPI";
    default:
      return "unknown";
  }
}

void WifiConfigApi::scanWifiNetworks(HTTPRequest* req, HTTPResponse* res) {
  req->discardRequestBody();

  int16_t numNetworksFound = WiFi.scanNetworks();

  JsonDocument respBody;
  JsonArray networks = respBody["networks"].to<JsonArray>();

  for (int i = 0; i < numNetworksFound; i++) {
    JsonObject networkRecord = networks.add<JsonObject>();
    networkRecord["ssid"] = WiFi.SSID(i);
    networkRecord["rssi"] = WiFi.RSSI(i);
    networkRecord["security"] =
        wifiEncryptionTypeToString(WiFi.encryptionType(i));
  }

  res->setStatusCode(200);
  res->setHeader(HttpHeader::ContentType, MimeType::JSON);
  serializeJson(respBody, *res);
}

void WifiConfigApi::saveWifiNetwork(HTTPRequest* req, HTTPResponse* res) {
  unique_ptr<char[]> reqBodyText = HttpUtil::readRequestBody(req, 200);

  if (reqBodyText == nullptr) {
    res->setStatusCode(413);
    res->println("Request entity too large");
    return;
  }

  JsonDocument reqBody;
  JsonDocument filter;
  filter["ssid"] = true;
  filter["passphrase"] = true;
  deserializeJson(reqBody, reqBodyText.get(),
                  DeserializationOption::Filter(filter));

  if (reqBody["ssid"].isUnbound() || reqBody["passphrase"].isUnbound()) {
    res->setStatusCode(400);
    res->setHeader(HttpHeader::ContentType, MimeType::JSON);
    res->println("{ \"success\": false }");
    return;
  }

  wifiConfigManager.putConfig(reqBody["ssid"].as<String>(),
                              reqBody["passphrase"].as<String>());
  res->setStatusCode(200);
  res->setHeader(HttpHeader::ContentType, MimeType::JSON);
  res->println("{ \"success\": true }");
}

void WifiConfigApi::getSavedWifiNetwork(HTTPRequest* req, HTTPResponse* res) {
  req->discardRequestBody();
  WifiConfig config = wifiConfigManager.getConfig();

  res->setStatusCode(200);
  res->setHeader(HttpHeader::ContentType, MimeType::JSON);
  if (!config.isConfigured) {
    res->println("{ \"ssid\": null }");
    return;
  }

  char respBody[50];
  sprintf(respBody, "{ \"ssid\": \"%s\" }", config.ssid.c_str());
  res->println(respBody);
}
