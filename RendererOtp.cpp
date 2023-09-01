#include "RendererOtp.h"

#include "Util.h"

OTPRenderer::OTPRenderer(TFT_eSPI *tftRef, ScreenManagerMutator *screenMutator):
  tft(tftRef), screenMutator(screenMutator), currentSecret(nullptr), totpGenerator(TOTP(nullptr, 0, TIMESTEP)) {}

void OTPRenderer::setCurrentSecret(Secret *newSecret) {
  if (currentSecret != nullptr) {
    free(currentSecret);
  }
  currentSecret = newSecret;
}

void OTPRenderer::clearCurrentSecret() {
  setCurrentSecret(nullptr);
}

bool OTPRenderer::isCurrentSecretValid() {
  return currentSecret != nullptr && currentSecret->isValid();
}

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
    secretManager.end();
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
    uint64_t otpCounter = TimeManager::getInstance()->getTime().unix() / TIMESTEP;
    if (otpCounter != oldOtpCounter) {
      oldOtpCounter = otpCounter;
      renderOTPValue();
    }

    int progBarPx = (TimeManager::getInstance()->getTime().unix() % TIMESTEP) * 4;
    tft->fillRect(16, 96, progBarPx, 10, TFT_GREEN);
  }
}

void OTPRenderer::handleTopButton() {
  curSecretIdx = (curSecretIdx + 1) % secretManager.getSecretCount();
  reloadOtpConfig();
  renderOTPInitial();
}

void OTPRenderer::handleBottomButton() {
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
  
  totpGenerator = TOTP((uint8_t *) currentSecret->get(), currentSecret->byteLen(), TIMESTEP);
  oldOtpCounter = TimeManager::getInstance()->getTime().unix() / TIMESTEP;
}

void OTPRenderer::handleBottomButtonLong() {
  secretManager.end();
  clearCurrentSecret();
  screenMutator->setState(MENU_STATE_MAIN);
}

void OTPRenderer::renderOTPInitial() {
  screenMutator->resetDrawConfig();

  tft->fillRect(16, 32, 224, 60, TFT_BLACK);
  tft->setTextDatum(TL_DATUM);

  if (!isCurrentSecretValid()) {
    tft->drawString("Secret at " + String(curSecretIdx) + " is invalid", 16, 32);
    return;
  }

  tft->drawString(currentSecret->getName().c_str(), 16, 32);
  screenMutator->resetDrawConfig();
  renderOTPValue();
}

void OTPRenderer::renderOTPValue() {
  long timestamp = TimeManager::getInstance()->getTime().unix();
  char *code = totpGenerator.getCodeFromTimestamp(timestamp);
  tft->setTextDatum(TL_DATUM);
  tft->fillRect(16, 52, 224, 40, TFT_BLACK);
  tft->setTextSize(4);
  tft->setTextColor(TFT_CYAN);
  tft->drawString(code, 16, 60);
  tft->fillRect(16, 96, 120, 10, TFT_BLACK);

  tft->fillRect(16, 115, 100, 20, TFT_BLACK);
  tft->setTextSize(1);
  tft->setTextColor(TFT_PINK);
  tft->drawString(String(timestamp), 16, 120);

  screenMutator->resetDrawConfig();
}
