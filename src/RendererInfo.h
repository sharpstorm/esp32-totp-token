#pragma once

#ifndef InfoRenderer_h
#define InfoRenderer_h

#include <M5StickC.h>

#include "IInputHandler.h"
#include "ScreenManagerMutator.h"
#include "Secret/SecretManager.h"

#define INFO_MENU_VOLTAGE 0
#define INFO_MENU_TIME 1
#define INFO_MENU_UNIX_TIME 2
#define INFO_MENU_SECRETS 3

class InfoRenderer : public IInputHandler {
 private:
  M5Display* tft;
  ScreenManagerMutator* screenMutator;
  SecretManager secretManager;
  int vref = 1100;
  uint64_t timeStamp = 0;
  uint8_t page = 0;

  void renderVoltage();
  void renderTime();
  void renderUnixTime();
  void renderSecrets();

 public:
  InfoRenderer(M5Display* tftRef, ScreenManagerMutator* screenMutator);

  void renderInit();
  void renderLoop();
  void handleTopButton();
  void handleBottomButton();
  void handleBottomButtonLong();
};

#endif
