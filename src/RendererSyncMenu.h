#pragma once

#ifndef SyncMenuRenderer_h
#define SyncMenuRenderer_h

#include <M5StickC.h>

#include "IInputHandler.h"
#include "ScreenManagerMutator.h"

#define S1_LINE_1 4
#define S1_LINE_2 16
#define S1_LINE_3 28
#define S1_LINE_4 40
#define S1_LINE_5 52
#define S1_LINE_6 64

#define S1_LINE_HEIGHT 12

class SyncMenuRenderer : public IInputHandler {
 private:
  M5Display* tft;
  ScreenManagerMutator* screenMutator;
  
  void syncTime();

 public:
  SyncMenuRenderer(M5Display* tftRef, ScreenManagerMutator* screenMutator);

  void renderInit();
  void renderLoop();
  void handleTopButton();
  void handleBottomButton();
  void handleBottomButtonLong();
};

#endif
