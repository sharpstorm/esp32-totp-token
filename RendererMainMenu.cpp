#include "RendererMainMenu.h"

MainMenuRenderer::MainMenuRenderer(TFT_eSPI *tftRef, ScreenManagerMutator *screenMutator):
  tft(tftRef), screenMutator(screenMutator) {}

void MainMenuRenderer::renderInit() {
  menuSelection = 0;
  isDirty = true;
  tft->setTextColor(TFT_GREEN);
  tft->setTextSize(2);
  
  tft->setTextDatum(TL_DATUM);
  tft->drawString("Sync Clock", 36, 32);
  tft->drawString("Gen OTP", 36, 52);
  tft->drawString("Config Token", 36, 72);
  tft->drawString("Token Info", 36, 92);

  screenMutator->drawButtonLabels("Option >", "Ok >");
}

void MainMenuRenderer::renderLoop() {
  if (isDirty) {
    tft->fillRect(16, 32, 20, 120, TFT_BLACK);
    tft->setTextDatum(TL_DATUM);
    tft->drawString(">", 16, 32 + (menuSelection * 20));

    isDirty = false;
  }
}

void MainMenuRenderer::handleTopButton() {
  menuSelection = (menuSelection + 1) % 4;
  isDirty = true;
}

void MainMenuRenderer::handleBottomButton() {
  switch (menuSelection) {
    case 0:
      screenMutator->setState(MENU_STATE_SYNC);
      break;
    case 1:
      screenMutator->setState(MENU_STATE_OTP);
      break;
    case 2:
      screenMutator->setState(MENU_STATE_CONFIG);
      break;
    case 3:
      screenMutator->setState(MENU_STATE_INFO);
      break;
    default:
      break;
  }
  
}

void MainMenuRenderer::handleBottomButtonLong() {
  
}
