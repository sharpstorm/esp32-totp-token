#pragma once

#ifndef ConfigRenderer_h
#define ConfigRenderer_h

#include <TFT_eSPI.h>
#include "ScreenManagerMutator.h"
#include "IInputHandler.h"

class ConfigRenderer: public IInputHandler {
  private:
    TFT_eSPI *tft;
    ScreenManagerMutator *screenMutator;

  public:
    ConfigRenderer(TFT_eSPI *tftRef, ScreenManagerMutator *screenMutator);

    void renderInit();
    void renderLoop();
    void handleTopButton();
    void handleBottomButton();
    void handleBottomButtonLong();
};

#endif
