#include "RendererConfig.h"

#include "Util.h"

ConfigRenderer::ConfigRenderer(TFT_eSPI *tftRef, ScreenManagerMutator *screenMutator):
  tft(tftRef), screenMutator(screenMutator) {}

void ConfigRenderer::renderInit() {
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(TFT_GREEN);
  tft->setTextSize(2);
  tft->drawString("Config Mode", 16, 32);
  
  screenMutator->drawButtonLabels("Option >", "Ok (Back) >");
}

void ConfigRenderer::renderLoop() {
}

void ConfigRenderer::handleTopButton() {
}

void ConfigRenderer::handleBottomButton() {
}

void ConfigRenderer::handleBottomButtonLong() {
  screenMutator->setState(MENU_STATE_MAIN);
}
