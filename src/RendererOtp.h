#pragma once

#ifndef OTPRenderer_h
#define OTPRenderer_h

#include <M5StickC.h>

#include "IInputHandler.h"
#include "ScreenManagerMutator.h"
#include "Secret/SecretManager.h"
#include "TOTP/TOTP.h"
#include "Time/TimeManager.h"

#define TIMESTEP 30

class OTPRenderer : public IInputHandler {
 private:
  M5Display* tft;
  ScreenManagerMutator* screenMutator;
  SecretManager secretManager;
  Secret* currentSecret;

  int curSecretIdx;
  TOTP totpGenerator;

  int oldOtpCounter = 0;
  bool isDisabled = false;

  void renderOTPInitial();
  void renderOTPValue();
  void reloadOtpConfig();

  void setCurrentSecret(Secret* newSecret);
  void clearCurrentSecret();
  bool isCurrentSecretValid();

 public:
  OTPRenderer(M5Display* tftRef, ScreenManagerMutator* screenMutator);

  void renderInit();
  void renderLoop();
  void handleTopButton();
  void handleBottomButton();
  void handleBottomButtonLong();
};

#endif
