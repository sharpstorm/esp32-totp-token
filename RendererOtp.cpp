#include "RendererOtp.h"

#include "Util.h"

OTPRenderer::OTPRenderer(TFT_eSPI *tftRef, ScreenManagerMutator *screenMutator):
  tft(tftRef), screenMutator(screenMutator), currentSecret(Secret(0)), totpGenerator(TOTP(nullptr, 0)) {}

void OTPRenderer::renderInit() {
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(TFT_GREEN);
  tft->fillScreen(TFT_BLACK);

  tft->setTextSize(2);
  if (!TimeManager::getInstance()->hasSynced()) {
    tft->drawString("Time has not synced yet.", 16, 32);
    tft->drawString("Sync time first.", 16, 52);
    Util::espDelay(3000);
    screenMutator->setState(MENU_STATE_MAIN);
    return;
  }

  secretManager.start();
  if (secretManager.getSecretCount() == 0) {
    tft->drawString("No secrets loaded.", 16, 32);
    Util::espDelay(3000);
    screenMutator->setState(MENU_STATE_MAIN);
    return;
  }

  curSecretIdx = 0;
  reloadOtpConfig();
  screenMutator->drawButtonLabels("Next >", "Prev (Exit) >");
  renderOTPInitial();
}

void OTPRenderer::renderLoop() {
  static uint64_t timeStamp = 0;

  if (millis() - timeStamp > 100) {
    timeStamp = millis();
    uint64_t otpCounter = TimeManager::getInstance()->getTime().unix() / 30;
    if (otpCounter != oldOtpCounter) {
      oldOtpCounter = otpCounter;
      renderOTPValue();
    }

    int progBarPx = (TimeManager::getInstance()->getTime().unix() % 30) * 4;
    tft->fillRect(16, 96, progBarPx, 10, TFT_GREEN);
  }
}

void OTPRenderer::handleTopButton() {
  curSecretIdx = (curSecretIdx + 1) % secretManager.getSecretCount();
  reloadOtpConfig();
}

void OTPRenderer::handleBottomButton() {
  curSecretIdx--;
  if (curSecretIdx < 0) {
    curSecretIdx = secretManager.getSecretCount() - 1;
  }
  reloadOtpConfig();
}

void OTPRenderer::reloadOtpConfig() {
  currentSecret = secretManager.readRecord(curSecretIdx);
  totpGenerator = TOTP((uint8_t *)currentSecret.get(), currentSecret.byteLen());
  oldOtpCounter = TimeManager::getInstance()->getTime().unix() / 30;
}

void OTPRenderer::handleBottomButtonLong() {
  secretManager.end();
  screenMutator->setState(MENU_STATE_MAIN);
}

void OTPRenderer::renderOTPInitial() {
  screenMutator->resetDrawConfig();

  tft->fillRect(16, 32, 224, 60, TFT_BLACK);
  tft->setTextDatum(TL_DATUM);
  tft->drawString(currentSecret.getName().c_str(), 16, 32);
  screenMutator->resetDrawConfig();
  renderOTPValue();
}

void OTPRenderer::renderOTPValue() {
  char *code = totpGenerator.getCode(TimeManager::getInstance()->getTime().unix());
  tft->setTextDatum(TL_DATUM);
  tft->fillRect(16, 52, 224, 40, TFT_BLACK);
  tft->setTextSize(4);
  tft->setTextColor(TFT_CYAN);
  tft->drawString(code, 16, 60);
  tft->fillRect(16, 96, 120, 10, TFT_BLACK);
}
