#pragma once

#ifndef InfoRenderer_h
#define InfoRenderer_h

#include <TFT_eSPI.h>
#include "ScreenManagerMutator.h"
#include "IInputHandler.h"

#define INFO_MENU_VOLTAGE 0
#define INFO_MENU_TIME    1

class InfoRenderer: public IInputHandler {
  private:
    TFT_eSPI *tft;
    ScreenManagerMutator *screenMutator;
    int vref = 1100;
    uint64_t timeStamp = 0;
    uint8_t page = 0;

    void getRefVoltage();
    void renderVoltage();
    void renderTime();

  public:
    InfoRenderer(TFT_eSPI *tftRef, ScreenManagerMutator *screenMutator);

    void renderInit();
    void renderLoop();
    void handleTopButton();
    void handleBottomButton();
    void handleBottomButtonLong();
};

#endif
