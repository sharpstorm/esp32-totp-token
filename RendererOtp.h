#pragma once

#ifndef OTPRenderer_h
#define OTPRenderer_h

#include <TFT_eSPI.h>
#include "ScreenManagerMutator.h"
#include "IInputHandler.h"

class OTPRenderer: public IInputHandler {
  private:
    TFT_eSPI *tft;
    ScreenManagerMutator *screenMutator;

  public:
    OTPRenderer(TFT_eSPI *tftRef, ScreenManagerMutator *screenMutator);

    void renderInit();
    void renderLoop();
    void handleTopButton();
    void handleBottomButton();
    void handleBottomButtonLong();
};

#endif
