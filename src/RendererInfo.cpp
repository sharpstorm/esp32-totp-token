#include "RendererInfo.h"

#include "Util/RendererUtil.h"
#include "time.h"

#define ADC_EN 14  // ADC_EN is the ADC detection enable port
#define ADC_PIN 34

InfoRenderer::InfoRenderer(M5Display* tftRef,
                           ScreenManagerMutator* screenMutator)
    : tft(tftRef), screenMutator(screenMutator) {}

void InfoRenderer::renderInit() {
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(TFT_GREEN);
  tft->setTextSize(1);

  screenMutator->drawOptionLabel("^ Back");
  timeStamp = 0;
  page = 0;
  secretManager.start();
}

void InfoRenderer::renderLoop() {
  if (millis() - timeStamp > 1000) {
    timeStamp = millis();

    switch (page) {
      case INFO_MENU_VOLTAGE:
        renderVoltage();
        break;
      case INFO_MENU_TIME:
        renderTime();
        break;
      case INFO_MENU_UNIX_TIME:
        renderUnixTime();
        break;
      case INFO_MENU_SECRETS:
        renderSecrets();
        break;
      default:
        break;
    }
  }
}

void InfoRenderer::renderVoltage() {
  tft->setTextDatum(TL_DATUM);

  tft->setTextColor(TFT_GREEN);
  tft->drawString("Battery Power:", 8, 24);
  tft->setTextColor(TFT_CYAN);
  tft->fillRect(8, 36, 172, 12, TFT_BLACK);
  tft->drawString(String(M5.Axp.GetBatPower()) + " mw", 8, 36);

  tft->setTextColor(TFT_GREEN);
  tft->drawString("Battery Voltage:", 8, 56);
  tft->setTextColor(TFT_CYAN);
  tft->fillRect(8, 68, 172, 12, TFT_BLACK);
  tft->drawString(String(M5.Axp.GetBatVoltage()) + " V", 8, 68);
}

void InfoRenderer::renderTime() {
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(TFT_PINK);
  tft->drawString("Time:", 8, 24);
  tft->setTextColor(TFT_CYAN);
  RendererUtil::drawLocalTime(tft, 8, 36);
}

void InfoRenderer::renderUnixTime() {
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(TFT_PINK);
  tft->drawString("Unix Time:", 8, 24);
  tft->setTextColor(TFT_CYAN);
  RendererUtil::drawUnixTime(tft, 8, 36);
}

void InfoRenderer::renderSecrets() {
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(TFT_PINK);
  tft->drawString("Secret Cnt:", 8, 24);
  tft->setTextColor(TFT_CYAN);
  tft->drawString(String(secretManager.getSecretCount()), 90, 24);
}

void InfoRenderer::handleTopButton() {
  secretManager.end();
  screenMutator->setState(MENU_STATE_MAIN);
}

void InfoRenderer::handleBottomButton() {
  page = (page + 1) % 4;
  tft->fillRect(0, 16, 180, 64, TFT_BLACK);
}

void InfoRenderer::handleBottomButtonLong() {}
