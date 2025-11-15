#pragma once

#ifndef ScreenManager_h
#define ScreenManager_h

#include <M5StickC.h>

#include "IInputHandler.h"
#include "RendererConfig.h"
#include "RendererInfo.h"
#include "RendererMainMenu.h"
#include "RendererOtp.h"
#include "RendererSyncMenu.h"
#include "ScreenManagerMutator.h"

// Dirty Bit Layout - 8 bits
// [4 bits - Global Command]
// [4 bits - State Command]

#define SCREEN_DIRTY_ALL 0x10
#define SCREEN_DIRTY_GLOBAL_MASK 0xF0
#define SCREEN_DIRTY_STATE_MASK 0xF

class ScreenManager : public ScreenManagerMutator {
 private:
  uint8_t screenDirtyState = 0;
  M5Display* tft;
  MainMenuRenderer mainMenuRenderer;
  SyncMenuRenderer syncMenuRenderer;
  OTPRenderer otpRenderer;
  ConfigRenderer configRenderer;
  InfoRenderer infoRenderer;

  IInputHandler* activeHandler;

  uint16_t savedColor;
  uint8_t savedTextSize;

 public:
  ScreenManager(M5Display* tftRef);

  void setState(int state) override;
  int getState();
  bool isInState(int state);

  void setGlobalDirty() override;
  bool isGlobalDirty();
  void resetGlobalDirty();

  void drawOptionLabel(const char* top) override;
  void resetDrawConfig() override;
  void saveDrawConfig() override;
  void restoreDrawConfig() override;

  // Passthrough methods
  void renderInit();
  void renderLoop();
  void handleTopButton();
  void handleBottomButton();
  void handleBottomButtonLong();
};

#endif
