#include "RendererMainMenu.h"

MainMenuRenderer::MainMenuRenderer(M5Display* tftRef,
                                   ScreenManagerMutator* screenMutator)
    : tft(tftRef), screenMutator(screenMutator) {}

void MainMenuRenderer::renderInit() {
  menuSelection = 0;
  isDirty = true;
  screenMutator->drawOptionLabel("^ Select");
}

void MainMenuRenderer::renderLoop() {
  if (isDirty) {
    tft->fillRect(0, 16, 160, 64, TFT_BLACK);
    tft->setTextDatum(TL_DATUM);
    tft->setTextColor(TFT_GREEN);
    tft->setTextSize(2);

    switch (menuSelection) {
      case 0:
        tft->drawString("Gen OTP", 8, 24);
        break;
      case 1:
        tft->drawString("Sync Clock", 8, 24);
        break;
      case 2:
        tft->drawString("Config Tool", 8, 24);
        break;
      case 3:
        tft->drawString("Token Info", 8, 24);
        break;
    }

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
      screenMutator->setState(MENU_STATE_OTP);
      break;
    case 1:
      screenMutator->setState(MENU_STATE_SYNC);
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

void MainMenuRenderer::handleBottomButtonLong() {}
