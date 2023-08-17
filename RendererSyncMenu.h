#pragma once

#ifndef SyncMenuRenderer_h
#define SyncMenuRenderer_h

#include <TFT_eSPI.h>
#include "ScreenManagerMutator.h"
#include "IInputHandler.h"

class SyncMenuRenderer: public IInputHandler {
  private:
    TFT_eSPI *tft;
    ScreenManagerMutator *screenMutator;

    bool attemptConnection();
    void turnWifiOff();
    void syncTime();

  public:
    SyncMenuRenderer(TFT_eSPI *tftRef, ScreenManagerMutator *screenMutator);

    void renderInit();
    void renderLoop();
    void handleTopButton();
    void handleBottomButton();
    void handleBottomButtonLong();
};

#endif
