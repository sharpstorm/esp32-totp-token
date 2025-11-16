#include "RendererWifiConfig.h"

#include "Time/TimeManager.h"
#include "Util/RendererUtil.h"
#include "Util/TimeUtil.h"
#include "Wifi/WifiManager.h"

WifiConfigRenderer::WifiConfigRenderer(M5Display* tftRef,
                                       ScreenManagerMutator* screenMutator)
    : tft(tftRef), screenMutator(screenMutator), appServer() {}

void WifiConfigRenderer::renderInit() {
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(TFT_GREEN);
  tft->setTextSize(1);

  tft->drawString("> Starting WiFi Hotspot", 8, S1_LINE_1);
  wifiManager.startWifiHotspot();
  appServer.start();
  dnsServer.start();
  tft->fillRect(0, S1_LINE_1, 180, S1_LINE_HEIGHT, TFT_BLACK);

  screenMutator->drawOptionLabel("^ Back");

  tft->drawString("> Hotspot SSID", 8, S1_LINE_1);
  tft->setTextColor(TFT_CYAN);
  tft->drawString(wifiManager.getConfiguredHotspotSsid(), 8, S1_LINE_2);

  tft->setTextColor(TFT_GREEN);
  tft->drawString("> Hotspot Password", 8, S1_LINE_3);
  tft->setTextColor(TFT_CYAN);
  tft->drawString(wifiManager.getConfiguredHotspotPassword(), 8, S1_LINE_4);

  tft->setTextColor(TFT_GREEN);
  tft->drawString("> IP Address", 8, S1_LINE_5);
  tft->setTextColor(TFT_CYAN);
  tft->drawString(wifiManager.getHotspotGatewayIp().toString(), 8, S1_LINE_6);
  tft->setTextColor(TFT_GREEN);
}

void WifiConfigRenderer::renderLoop() {
  appServer.loop();
  dnsServer.loop();
}

void WifiConfigRenderer::handleTopButton() {
  dnsServer.stop();
  appServer.stop();
  wifiManager.stopWifiHotspot();
  screenMutator->setState(MENU_STATE_MAIN);
}

void WifiConfigRenderer::handleBottomButton() {}

void WifiConfigRenderer::handleBottomButtonLong() {}
