#include "RendererSyncMenu.h"

#include "WiFi.h"
#include "time.h"
#include "util.h"
#include "Config.h"

#define NTP_SERVER      "pool.ntp.org"
#define GMT_OFFSET_SECS 28800

SyncMenuRenderer::SyncMenuRenderer(TFT_eSPI *tftRef, ScreenManagerMutator *screenMutator):
  tft(tftRef), screenMutator(screenMutator) {}

void SyncMenuRenderer::renderInit() {
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(TFT_GREEN);
  tft->setTextSize(2);

  tft->drawString("> Connecting to ", 16, 32);
  tft->drawString(WIFI_SSID, 16, 52);

  bool success = attemptConnection();
  if (!success) {
    turnWifiOff();

    tft->fillRect(16, 32, 200, 40, TFT_BLACK);
    tft->setTextColor(TFT_RED);
    tft->drawString("Failed to connect to ", 16, 32);
    tft->drawString(WIFI_SSID, 16, 52);

    Util::espDelay(3000);
    screenMutator->setState(MENU_STATE_MAIN);
    return;
  }

  tft->fillRect(16, 32, 200, 40, TFT_BLACK);
  tft->setTextColor(TFT_CYAN);
  tft->drawString("Connected to ", 16, 32);
  tft->drawString(WIFI_SSID, 16, 52);
  tft->setTextColor(TFT_GREEN);
  tft->drawString("> Syncing Time", 16, 72);

  configTime(GMT_OFFSET_SECS, 0, NTP_SERVER);
  Util::drawLocalTime(tft, 16, 92);
  tft->fillRect(16, 72, 200, 20, TFT_BLACK);
  tft->setTextColor(TFT_CYAN);
  tft->drawString("Time Now: ", 16, 72);
  tft->setTextColor(TFT_GREEN);
  
  Util::espDelay(500);
  turnWifiOff();

  Util::espDelay(2000);
  screenMutator->setState(MENU_STATE_MAIN);
}

bool SyncMenuRenderer::attemptConnection() {
  uint64_t timeSinceConnection = millis();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (millis() - timeSinceConnection > 5000) {
      return false;
    }
  }

  return true;
}

void SyncMenuRenderer::turnWifiOff() {
  WiFi.disconnect(true);
  WiFi.setSleep(true);
  WiFi.mode(WIFI_OFF);
}

void SyncMenuRenderer::renderLoop() {
}

void SyncMenuRenderer::handleTopButton() {
}

void SyncMenuRenderer::handleBottomButton() {
}

void SyncMenuRenderer::handleBottomButtonLong() {
  
}
