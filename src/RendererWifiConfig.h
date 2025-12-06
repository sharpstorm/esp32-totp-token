#pragma once

#ifndef WifiConfigRenderer_h
#define WifiConfigRenderer_h

#include <M5StickC.h>

#include "AppServer/AppServer.h"
#include "IInputHandler.h"
#include "ScreenManagerMutator.h"

#define S1_LINE_1 4
#define S1_LINE_2 16
#define S1_LINE_3 28
#define S1_LINE_4 40
#define S1_LINE_5 52
#define S1_LINE_6 64

#define S1_LINE_HEIGHT 12

struct AppServerWorkerDto {
  AppServer* appServer;
  bool isRunning;
};

class WifiConfigRenderer : public IInputHandler {
 private:
  M5Display* tft;
  ScreenManagerMutator* screenMutator;
  AppServer appServer;
  AppServerWorkerDto appServerState;

  void syncTime();
  static void appServerWorker(void* pvParameters);

 public:
  WifiConfigRenderer(M5Display* tftRef, ScreenManagerMutator* screenMutator);

  void renderInit();
  void renderLoop();
  void handleTopButton();
  void handleBottomButton();
  void handleBottomButtonLong();
};

#endif
