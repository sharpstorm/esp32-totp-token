#pragma once

#ifndef RendererBtConfig_h
#define RendererBtConfig_h

#include <M5StickC.h>

#include "BLE/BleConfigManager.h"
#include "IInputHandler.h"
#include "ScreenManagerMutator.h"

#define BT_SCREEN_PIN 0
#define BT_SCREEN_QR 1
#define BT_SCREEN_ACTIVE 2

class BtConfigRenderer : public IInputHandler {
 private:
  M5Display* tft;
  ScreenManagerMutator* screenMutator;
  BleConfigManager bleManager;

  uint8_t screenPage;
  uint8_t lastBleState;
  uint64_t lastUpdateTime;
  bool showHex;

  void drawPinScreen();
  void drawQrScreen();
  void drawHexScreen();
  void drawActiveScreen();
  void drawQrCode(const char* data, int originX, int originY, int moduleSize);

 public:
  BtConfigRenderer(M5Display* tftRef, ScreenManagerMutator* screenMutator);

  void renderInit();
  void renderLoop();
  void handleTopButton();
  void handleBottomButton();
  void handleBottomButtonLong();
};

#endif
