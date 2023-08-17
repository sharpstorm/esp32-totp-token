#pragma once

#ifndef MainMenuRenderer_h
#define MainMenuRenderer_h

#include <TFT_eSPI.h>
#include "ScreenManagerMutator.h"
#include "IInputHandler.h"

class MainMenuRenderer: public IInputHandler {
  private:
    TFT_eSPI *tft;
    ScreenManagerMutator *screenMutator;
    int menuSelection = 0;

    bool isDirty = false;

  public:
    MainMenuRenderer(TFT_eSPI *tftRef, ScreenManagerMutator *screenMutator);

    void renderInit();
    void renderLoop();
    void handleTopButton();
    void handleBottomButton();
    void handleBottomButtonLong();
};

#endif
