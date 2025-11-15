#include "RendererOtp.h"

OTPRenderer::OTPRenderer(M5Display* tftRef, ScreenManagerMutator* screenMutator)
    : tft(tftRef),
      screenMutator(screenMutator),
      currentSecret(nullptr),
      totpGenerator(TOTP(nullptr, 0, TIMESTEP)) {}

void OTPRenderer::setCurrentSecret(Secret* newSecret) {
  if (currentSecret != nullptr) {
    free(currentSecret);
  }
  currentSecret = newSecret;
}

void OTPRenderer::clearCurrentSecret() { setCurrentSecret(nullptr); }

bool OTPRenderer::isCurrentSecretValid() {
  return currentSecret != nullptr && currentSecret->isValid();
}

void OTPRenderer::renderInit() {
  tft->setTextColor(TFT_GREEN);
  tft->fillScreen(TFT_BLACK);
  screenMutator->drawOptionLabel("^ Settings");

  tft->setTextSize(2);
  tft->setTextDatum(TL_DATUM);

  secretManager.start();
  if (secretManager.getSecretCount() == 0) {
    tft->drawString("No secrets", 8, 16);
    tft->drawString("loaded.", 8, 32);
    secretManager.end();
    isDisabled = true;
    return;
  }

  isDisabled = false;
  curSecretIdx = 0;
  reloadOtpConfig();
  renderOTPInitial();
}

void OTPRenderer::renderLoop() {
  static uint64_t timeStamp = 0;

  if (isDisabled) {
    return;
  }

  if (millis() - timeStamp > 100) {
    timeStamp = millis();
    uint64_t otpCounter =
        TimeManager::getInstance()->getTime().unix() / TIMESTEP;
    if (otpCounter != oldOtpCounter) {
      oldOtpCounter = otpCounter;
      renderOTPValue();
    }

    int progBarPx =
        (TimeManager::getInstance()->getTime().unix() % TIMESTEP) * 4;
    tft->fillRect(4, 62, progBarPx, 4, TFT_GREEN);
  }
}

void OTPRenderer::handleTopButton() {
  secretManager.end();
  clearCurrentSecret();
  screenMutator->setState(MENU_STATE_MAIN);
}

void OTPRenderer::handleBottomButton() {
  curSecretIdx = (curSecretIdx + 1) % secretManager.getSecretCount();
  reloadOtpConfig();
  renderOTPInitial();
}

void OTPRenderer::handleBottomButtonLong() {
  curSecretIdx--;
  if (curSecretIdx < 0) {
    curSecretIdx = secretManager.getSecretCount() - 1;
  }
  reloadOtpConfig();
  renderOTPInitial();
}

void OTPRenderer::reloadOtpConfig() {
  Secret sec = secretManager.readRecord(curSecretIdx);
  setCurrentSecret(sec.isValid() ? new Secret(sec) : nullptr);

  if (!isCurrentSecretValid()) {
    return;
  }

  totpGenerator =
      TOTP((uint8_t*)currentSecret->get(), currentSecret->byteLen(), TIMESTEP);
  oldOtpCounter = TimeManager::getInstance()->getTime().unix() / TIMESTEP;
}

void OTPRenderer::renderOTPInitial() {
  screenMutator->resetDrawConfig();

  tft->fillRect(8, 16, 172, 64, TFT_BLACK);
  tft->setTextDatum(TL_DATUM);

  if (!isCurrentSecretValid()) {
    tft->setTextSize(1);
    tft->drawString("Secret at " + String(curSecretIdx) + " is invalid", 8, 16);
    return;
  }

  tft->drawString(currentSecret->getName().c_str(), 8, 16);
  screenMutator->resetDrawConfig();
  renderOTPValue();
}

void OTPRenderer::renderOTPValue() {
  long timestamp = TimeManager::getInstance()->getTime().unix();
  char* code = totpGenerator.getCodeFromTimestamp(timestamp);
  tft->setTextDatum(TL_DATUM);
  tft->fillRect(8, 32, 180, 32, TFT_BLACK);
  tft->setTextSize(3);
  tft->setTextColor(TFT_CYAN);
  tft->drawString(code, 8, 34);
  tft->fillRect(4, 62, 120, 4, TFT_BLACK);

  tft->fillRect(4, 68, 176, 12, TFT_BLACK);
  tft->setTextSize(1);
  tft->setTextColor(TFT_PINK);

  tft->setTextDatum(TR_DATUM);
  tft->drawString(String(timestamp), 160, 68);

  screenMutator->resetDrawConfig();
}
