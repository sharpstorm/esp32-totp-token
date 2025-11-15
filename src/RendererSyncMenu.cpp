#include "RendererSyncMenu.h"

#include "Time/TimeManager.h"
#include "Util/RendererUtil.h"
#include "Util/TimeUtil.h"
#include "Wifi/WifiManager.h"

SyncMenuRenderer::SyncMenuRenderer(M5Display* tftRef,
                                   ScreenManagerMutator* screenMutator)
    : tft(tftRef), screenMutator(screenMutator) {}

void SyncMenuRenderer::renderInit() {
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(TFT_GREEN);
  tft->setTextSize(1);

  tft->drawString("> Connecting to ", 8, S1_LINE_1);
  tft->drawString(wifiManager.getConfiguredSsid(), 8, S1_LINE_2);

  bool success = wifiManager.connectToWifi();
  if (!success) {
    wifiManager.disconnectWifi();

    tft->fillRect(8, S1_LINE_1, 180, S1_LINE_HEIGHT, TFT_BLACK);
    tft->setTextColor(TFT_RED);
    tft->drawString("Failed to connect to ", 8, S1_LINE_1);

    TimeUtil::espDelay(3000);
    screenMutator->setState(MENU_STATE_MAIN);
    return;
  }

  tft->fillRect(8, S1_LINE_1, 180, S1_LINE_HEIGHT, TFT_BLACK);
  tft->setTextColor(TFT_CYAN);
  tft->drawString("Connected to ", 8, S1_LINE_1);
  tft->setTextColor(TFT_GREEN);
  tft->drawString("> Syncing Time", 8, S1_LINE_3);

  if (!TimeManager::getInstance()->syncTime()) {
    tft->fillRect(8, S1_LINE_3, 180, S1_LINE_HEIGHT, TFT_BLACK);
    tft->setTextColor(TFT_RED);
    tft->drawString("NTP Sync Failed", 8, S1_LINE_3);

    TimeUtil::espDelay(3000);
    screenMutator->setState(MENU_STATE_MAIN);
    return;
  }

  RendererUtil::drawLocalTime(tft, 16, S1_LINE_4);
  tft->fillRect(8, S1_LINE_3, 180, S1_LINE_HEIGHT, TFT_BLACK);
  tft->setTextColor(TFT_CYAN);
  tft->drawString("Time Now: ", 8, S1_LINE_3);
  tft->setTextColor(TFT_GREEN);

  TimeUtil::espDelay(500);
  wifiManager.disconnectWifi();

  TimeUtil::espDelay(2000);
  screenMutator->setState(MENU_STATE_MAIN);
}

void SyncMenuRenderer::renderLoop() {}

void SyncMenuRenderer::handleTopButton() {}

void SyncMenuRenderer::handleBottomButton() {}

void SyncMenuRenderer::handleBottomButtonLong() {}
