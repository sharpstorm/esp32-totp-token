#pragma once

#ifndef MainMenuRenderer_h
#define MainMenuRenderer_h

#include <M5StickC.h>

#include "IInputHandler.h"
#include "ScreenManagerMutator.h"

class MainMenuRenderer : public IInputHandler {
 private:
  M5Display* tft;
  ScreenManagerMutator* screenMutator;
  int menuSelection = 0;

  bool isDirty = false;

 public:
  MainMenuRenderer(M5Display* tftRef, ScreenManagerMutator* screenMutator);

  void renderInit();
  void renderLoop();
  void handleTopButton();
  void handleBottomButton();
  void handleBottomButtonLong();
};

#endif
