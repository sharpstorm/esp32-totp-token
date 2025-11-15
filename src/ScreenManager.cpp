#include "ScreenManager.h"

ScreenManager::ScreenManager(M5Display* tftRef)
    : tft(tftRef),
      mainMenuRenderer(tftRef, this),
      syncMenuRenderer(tftRef, this),
      otpRenderer(tftRef, this),
      configRenderer(tftRef, this),
      infoRenderer(tftRef, this),
      activeHandler(&mainMenuRenderer) {}

void ScreenManager::setState(int state) {
  screenDirtyState &= (~SCREEN_DIRTY_STATE_MASK);
  screenDirtyState |= (state & SCREEN_DIRTY_STATE_MASK);

  switch (state) {
    case MENU_STATE_MAIN:
      activeHandler = &mainMenuRenderer;
      break;
    case MENU_STATE_SYNC:
      activeHandler = &syncMenuRenderer;
      break;
    case MENU_STATE_OTP:
      activeHandler = &otpRenderer;
      break;
    case MENU_STATE_CONFIG:
      activeHandler = &configRenderer;
      break;
    case MENU_STATE_INFO:
      activeHandler = &infoRenderer;
      break;
    default:
      activeHandler = &mainMenuRenderer;
      break;
  }

  setGlobalDirty();
}

int ScreenManager::getState() {
  return (screenDirtyState & SCREEN_DIRTY_STATE_MASK);
}

bool ScreenManager::isInState(int state) { return getState() == state; }

void ScreenManager::setGlobalDirty() { screenDirtyState |= SCREEN_DIRTY_ALL; }

bool ScreenManager::isGlobalDirty() {
  return (screenDirtyState & SCREEN_DIRTY_ALL) > 0;
}

void ScreenManager::resetGlobalDirty() {
  screenDirtyState &= (~SCREEN_DIRTY_GLOBAL_MASK);
}

void ScreenManager::drawOptionLabel(const char* top) {
  uint16_t oldColor = tft->textcolor;
  uint8_t oldSize = tft->textsize;

  tft->setTextColor(TFT_PINK);
  tft->setTextSize(1);

  tft->setTextDatum(TR_DATUM);
  tft->drawString(top, tft->width(), 0);

  tft->setTextSize(oldSize);
  tft->setTextColor(oldColor);
}

void ScreenManager::resetDrawConfig() {
  tft->setTextSize(2);
  tft->setTextColor(TFT_GREEN);
}

void ScreenManager::saveDrawConfig() {
  savedColor = tft->textcolor;
  savedTextSize = tft->textsize;
}

void ScreenManager::restoreDrawConfig() {
  tft->setTextSize(savedTextSize);
  tft->setTextColor(savedColor);
}

void ScreenManager::renderInit() {
  if (activeHandler == nullptr) {
    return;
  }
  activeHandler->renderInit();
}

void ScreenManager::renderLoop() {
  switch (getState()) {
    case MENU_STATE_MAIN:
      mainMenuRenderer.renderLoop();
      break;
    case MENU_STATE_OTP:
      otpRenderer.renderLoop();
      break;
    case MENU_STATE_CONFIG:
      configRenderer.renderLoop();
      break;
    case MENU_STATE_INFO:
      infoRenderer.renderLoop();
    default:
      break;
  }
}

void ScreenManager::handleTopButton() {
  if (activeHandler == nullptr) {
    return;
  }
  activeHandler->handleTopButton();
}

void ScreenManager::handleBottomButton() {
  if (activeHandler == nullptr) {
    return;
  }
  activeHandler->handleBottomButton();
}

void ScreenManager::handleBottomButtonLong() {
  if (activeHandler == nullptr) {
    return;
  }
  activeHandler->handleBottomButtonLong();
}
