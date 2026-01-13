#pragma once

#ifndef ConfigRenderer_h
#define ConfigRenderer_h

#include <M5StickC.h>

#include "IInputHandler.h"
#include "ScreenManagerMutator.h"
#include "Secret/SecretManager.h"

#define COMMAND_WORD_START 0xDE
#define COMMAND_WORD_END 0xA0

#define CONFIG_STATE_COMMAND_PRE 1
#define CONFIG_STATE_COMMAND_READ 2
#define CONFIG_STATE_COMMAND_POST 3
#define CONFIG_STATE_COMMAND_EXEC 4
#define CONFIG_STATE_WAIT_CLEAR 5

#define COMMAND_WORD_DEBUG 1
#define COMMAND_WORD_SHOW 2
#define COMMAND_WORD_DELETE 3
#define COMMAND_WORD_PUT 4

class ConfigRenderer : public IInputHandler {
 private:
  M5Display* tft;
  ScreenManagerMutator* screenMutator;

  byte buffer[4];
  void* bufPtr;
  uint8_t serialState;
  uint8_t command;
  uint64_t timestamp = 0;

  void handleCommandRead();
  void handleCommandInit();
  void handleCommandExec();
  void resetScreen();

  void handleDebug();
  void handleShow();
  void handleCreate();
  void handleDelete();

 public:
  ConfigRenderer(M5Display* tftRef, ScreenManagerMutator* screenMutator);

  void renderInit();
  void renderLoop();
  void handleTopButton();
  void handleBottomButton();
  void handleBottomButtonLong();
};

#endif
