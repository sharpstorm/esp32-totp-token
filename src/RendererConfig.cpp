#include "RendererConfig.h"

#include "Util/Base32.h"

ConfigRenderer::ConfigRenderer(M5Display* tftRef,
                               ScreenManagerMutator* screenMutator)
    : tft(tftRef), screenMutator(screenMutator) {}

void ConfigRenderer::renderInit() {
  screenMutator->resetDrawConfig();
  tft->setTextDatum(TL_DATUM);
  tft->drawString("Config Mode", 8, 14);
  tft->setTextColor(TFT_PINK);
  tft->drawString("Connecting..", 8, 32);

  screenMutator->drawOptionLabel("^ Back");
  memset(&buffer, 0, sizeof(buffer));
  serialState = CONFIG_STATE_COMMAND_PRE;
  command = 0;
  secretManager.start();
}

void ConfigRenderer::renderLoop() {
  switch (serialState) {
    case CONFIG_STATE_COMMAND_PRE:
    case CONFIG_STATE_COMMAND_READ:
    case CONFIG_STATE_COMMAND_POST:
      handleCommandRead();
      break;
    case CONFIG_STATE_COMMAND_EXEC:
      handleCommandExec();
      break;
    case CONFIG_STATE_WAIT_CLEAR:
      if (millis() - timestamp > 3000) {
        serialState = CONFIG_STATE_COMMAND_PRE;
        resetScreen();
      }
      break;
    default:
      break;
  }
}

void ConfigRenderer::handleTopButton() {
  if (bufPtr != nullptr) {
    free(bufPtr);
  }
  secretManager.end();
  screenMutator->setState(MENU_STATE_MAIN);
}

void ConfigRenderer::handleBottomButton() {}

void ConfigRenderer::handleBottomButtonLong() {}

void ConfigRenderer::handleCommandRead() {
  if (Serial.available() == 0) {
    return;
  }

  uint8_t byte = Serial.read();
  switch (serialState) {
    case CONFIG_STATE_COMMAND_PRE:
      if (byte == COMMAND_WORD_START) {
        serialState = CONFIG_STATE_COMMAND_READ;
      }
      break;

    case CONFIG_STATE_COMMAND_READ:
      if (byte < 32) {
        serialState = CONFIG_STATE_COMMAND_POST;
        command = byte;
      } else {
        serialState = CONFIG_STATE_COMMAND_PRE;
      }
      break;

    case CONFIG_STATE_COMMAND_POST:
      if (byte == COMMAND_WORD_END) {
        serialState = CONFIG_STATE_COMMAND_EXEC;
        handleCommandInit();
      } else {
        serialState = CONFIG_STATE_COMMAND_PRE;
      }
      break;

    default:
      serialState = CONFIG_STATE_COMMAND_PRE;
      break;
  }
}

void ConfigRenderer::handleCommandInit() {
  screenMutator->resetDrawConfig();
  tft->setTextDatum(TL_DATUM);
  tft->fillRect(0, 32, 180, 50, TFT_BLACK);
  switch (command) {
    case COMMAND_WORD_DEBUG:
      tft->drawString("Debug Recv", 8, 32);
      break;
    case COMMAND_WORD_PUT:
    case COMMAND_WORD_DELETE:
    case COMMAND_WORD_SHOW:
      tft->drawString("Waiting", 8, 32);
      buffer[0] = 0;
      buffer[1] = 0;
      buffer[2] = 0;
      break;
    default:
      tft->drawString("Unknown Cmd", 8, 32);
      timestamp = millis();
      break;
  }
}

void ConfigRenderer::handleCommandExec() {
  switch (command) {
    case COMMAND_WORD_SHOW:
      handleShow();
      break;
    case COMMAND_WORD_PUT:
      handleCreate();
      break;
    case COMMAND_WORD_DELETE:
      handleDelete();
      break;
    case COMMAND_WORD_DEBUG:
    default:
      handleDebug();
      break;
  }
}

void ConfigRenderer::handleDebug() {
  timestamp = millis();
  serialState = CONFIG_STATE_WAIT_CLEAR;
}

void ConfigRenderer::handleShow() {
  // Waiting for index to show
  if (Serial.available() == 0) {
    return;
  }

  screenMutator->resetDrawConfig();
  tft->setTextDatum(TL_DATUM);
  tft->fillRect(0, 32, 180, 50, TFT_BLACK);
  tft->setTextSize(1);
  uint8_t index = Serial.read();
  Secret sec = secretManager.readRecord(index);
  if (sec.isValid()) {
    byte* secretString;
    Base32::toBase32(sec.get(), sec.bitLen(), secretString);
    tft->drawString("Secret:", 8, 32);
    tft->drawString(String(index).c_str(), 70, 32);
    tft->drawString(sec.getName().c_str(), 8, 44);
    tft->drawString((char*)secretString, 8, 56);
    free(secretString);
  } else {
    tft->drawString("Invalid Index", 8, 32);
  }

  timestamp = millis();
  serialState = CONFIG_STATE_WAIT_CLEAR;
}

void ConfigRenderer::handleCreate() {
  if (Serial.available() == 0) {
    return;
  }

  if (buffer[0] == 0) {
    // Waiting for index to show
    buffer[0] = Serial.read();
    return;
  }

  if (buffer[1] == 0) {
    buffer[1] = Serial.read();
    buffer[2] = 0;
    bufPtr = malloc(buffer[0] + buffer[1]);
    return;
  }

  byte buf[32];
  int bytesToRead = min(32, buffer[0] + buffer[1] - buffer[2]);
  size_t bytesRead = Serial.read(buf, min(bytesToRead, Serial.available()));
  int baseIdx = buffer[2];

  for (int i = 0; i < bytesRead; i++) {
    ((byte*)bufPtr)[baseIdx + i] = buf[i];
  }
  buffer[2] += bytesRead;

  if (buffer[2] < buffer[0] + buffer[1]) {
    return;
  }

  byte* secretBytes;
  char* nameBytes;
  Base32::fromBase32((byte*)bufPtr, buffer[0], secretBytes);

  Secret secret(buffer[0] * 5);
  memcpy(secret.get(), secretBytes, secret.byteLen());
  free(secretBytes);

  nameBytes = (char*)malloc(buffer[1] + 1);
  memcpy(nameBytes, ((byte*)bufPtr) + buffer[0], buffer[1]);
  nameBytes[buffer[1]] = 0;

  secret.setName(String(nameBytes));
  secretManager.putRecord(&secret);
  free(nameBytes);

  tft->fillRect(0, 32, 180, 50, TFT_BLACK);
  tft->setTextSize(1);
  tft->drawString("Added New Secret:", 8, 32);
  tft->drawString(String(secret.byteLen()).c_str(), 8, 44);
  tft->drawString("bytes, Pos:", 40, 44);
  tft->drawString(String(secretManager.getSecretCount() - 1).c_str(), 140, 44);
  Base32::toBase32(secret.get(), secret.bitLen(), secretBytes);
  tft->drawString(nameBytes, 8, 56);
  free(secretBytes);

  free(bufPtr);
  bufPtr = nullptr;

  timestamp = millis();
  serialState = CONFIG_STATE_WAIT_CLEAR;
}

void ConfigRenderer::handleDelete() {
  // Waiting for index to show
  if (Serial.available() == 0) {
    return;
  }

  secretManager.deleteRecord(Serial.read());
  screenMutator->resetDrawConfig();
  tft->setTextDatum(TL_DATUM);
  tft->fillRect(0, 32, 180, 50, TFT_BLACK);
  tft->setTextSize(1);
  tft->drawString("Secret Deleted", 8, 32);

  timestamp = millis();
  serialState = CONFIG_STATE_WAIT_CLEAR;
}

void ConfigRenderer::resetScreen() {
  screenMutator->resetDrawConfig();
  tft->setTextDatum(TL_DATUM);
  tft->fillRect(0, 32, 180, 50, TFT_BLACK);
  tft->setTextSize(1);
  tft->drawString("Waiting for Cmd", 8, 32);
}
