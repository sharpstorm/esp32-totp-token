#include "RendererConfig.h"

#include "Util.h"
#include "src/Secret/Base32.h"

ConfigRenderer::ConfigRenderer(TFT_eSPI *tftRef, ScreenManagerMutator *screenMutator):
  tft(tftRef), screenMutator(screenMutator) {}

void ConfigRenderer::renderInit() {
  screenMutator->resetDrawConfig();
  tft->setTextDatum(TL_DATUM);
  tft->drawString("Config Mode", 16, 32);
  tft->drawString("Connect Serial Tool", 16, 52);
  
  screenMutator->drawButtonLabels("Option >", "Ok (Back) >");
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
}

void ConfigRenderer::handleBottomButton() {
}

void ConfigRenderer::handleBottomButtonLong() {
  if (bufPtr != nullptr) {
    free(bufPtr);
  }
  secretManager.end();
  screenMutator->setState(MENU_STATE_MAIN);
}

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
  tft->fillRect(0, 52, 240, 20, TFT_BLACK);
  switch (command) {
    case COMMAND_WORD_DEBUG:
      tft->drawString("Serial Debug Cmd", 16, 52);
      break;
    case COMMAND_WORD_PUT:
    case COMMAND_WORD_DELETE:
    case COMMAND_WORD_SHOW:
      tft->drawString("Waiting", 16, 52);
      buffer[0] = 0;
      buffer[1] = 0;
      buffer[2] = 0;
      break;
    default:
      tft->drawString("Unknown Command", 16, 52);
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
  tft->fillRect(0, 52, 240, 20, TFT_BLACK);
  uint8_t index = Serial.read();
  Secret sec = secretManager.readRecord(index);
  if (sec.isValid()) {
    byte* secretString;
    Base32::toBase32(sec.get(), sec.bitLen(), secretString);
    tft->drawString("Secret:", 16, 52);
    tft->drawString(String(index).c_str(), 100, 52);
    tft->drawString(sec.getName().c_str(), 16, 72);
    tft->drawString((char *) secretString, 16, 92);
    free(secretString);
  } else {
    tft->drawString("Invalid Index", 16, 52);
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
    ((byte *) bufPtr)[baseIdx + i] = buf[i];
  }
  buffer[2] += bytesRead;

  if (buffer[2] < buffer[0] + buffer[1]) {
    return;
  }

  byte *secretBytes;
  char *nameBytes;
  Base32::fromBase32((byte*) bufPtr, buffer[0], secretBytes);

  Secret secret(buffer[0] * 5);
  memcpy(secret.get(), secretBytes, secret.byteLen());
  free(secretBytes);
  
  nameBytes = (char *) malloc(buffer[1] + 1);
  memcpy(nameBytes, ((byte*) bufPtr) + buffer[0], buffer[1]);
  nameBytes[buffer[1]] = 0;

  secret.setName(String(nameBytes));
  secretManager.putRecord(&secret);
  free(nameBytes);
  
  tft->fillRect(0, 52, 240, 20, TFT_BLACK);
  tft->drawString("Added New Secret:", 16, 52);
  tft->drawString(String(secret.byteLen()).c_str(), 16, 72);
  tft->drawString("bytes, Pos:", 40, 72);
  tft->drawString(String(secretManager.getSecretCount() - 1).c_str(), 190, 72);
  Base32::toBase32(secret.get(), secret.bitLen(), secretBytes);
  tft->drawString(nameBytes, 16, 92);
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
    tft->fillRect(0, 52, 240, 20, TFT_BLACK);
    tft->drawString("Secret Deleted", 16, 52);

    timestamp = millis();
    serialState = CONFIG_STATE_WAIT_CLEAR;
}

void ConfigRenderer::resetScreen() {
  screenMutator->resetDrawConfig();
  tft->setTextDatum(TL_DATUM);
  tft->fillRect(0, 52, 240, 60, TFT_BLACK);
  tft->drawString("Waiting for Cmd", 16, 52);
}

