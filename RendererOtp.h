#pragma once

#ifndef OTPRenderer_h
#define OTPRenderer_h

#include <TFT_eSPI.h>
#include "ScreenManagerMutator.h"
#include "IInputHandler.h"
#include "src/Secret/SecretManager.h"
#include "src/TOTP/TOTP.h"
#include "src/Time/TimeManager.h"

class OTPRenderer: public IInputHandler {
  private:
    TFT_eSPI *tft;
    ScreenManagerMutator *screenMutator;
    SecretManager secretManager;
    Secret currentSecret;
    int curSecretIdx;
    TOTP totpGenerator;

    int oldOtpCounter = 0;

    void renderOTPInitial();
    void renderOTPValue();
    void reloadOtpConfig();

  public:
    OTPRenderer(TFT_eSPI *tftRef, ScreenManagerMutator *screenMutator);

    void renderInit();
    void renderLoop();
    void handleTopButton();
    void handleBottomButton();
    void handleBottomButtonLong();
};

#endif
