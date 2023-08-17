#include "RendererOtp.h"

#include "Util.h"

OTPRenderer::OTPRenderer(TFT_eSPI *tftRef, ScreenManagerMutator *screenMutator):
  tft(tftRef), screenMutator(screenMutator) {}

void OTPRenderer::renderInit() {
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(TFT_GREEN);
  tft->fillScreen(TFT_BLACK);
  
  tft->setTextSize(2);
  tft->drawString("MapleSea", 16, 32);

  tft->setTextSize(4);
  tft->setTextColor(TFT_CYAN);
  tft->drawString("000000", 16, 60);

  screenMutator->drawButtonLabels("Option >", "Ok (Back) >");

  tft->setTextColor(TFT_GREEN);
  tft->setTextSize(2);
}

void OTPRenderer::renderLoop() {
  static uint8_t progressBar = 0;
  static uint64_t timeStamp = 0;
  if (millis() - timeStamp > 100) {
    timeStamp = millis();
    tft->fillRect(16, 96, 100, 10, TFT_BLACK);
    tft->fillRect(16, 96, progressBar, 10, TFT_GREEN);
    progressBar = (progressBar + 1) % 100;
  }
}

void OTPRenderer::handleTopButton() {
}

void OTPRenderer::handleBottomButton() {
}

void OTPRenderer::handleBottomButtonLong() {
  screenMutator->setState(MENU_STATE_MAIN);
}
