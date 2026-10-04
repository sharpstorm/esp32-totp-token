#include "RendererBtConfig.h"

#include "Secret/SecretManager.h"
#include "Util/QrCode.h"
#include "Wifi/WifiManager.h"

BtConfigRenderer::BtConfigRenderer(M5Display* tftRef,
                                   ScreenManagerMutator* screenMutator)
    : tft(tftRef),
      screenMutator(screenMutator),
      screenPage(BT_SCREEN_PIN),
      lastBleState(BLE_STATE_IDLE),
      lastUpdateTime(0),
      showHex(false) {}

void BtConfigRenderer::renderInit() {
  // Ensure WiFi radio is off before starting BLE
  wifiManager.disconnectWifi();

  secretManager.start();
  bleManager.start();

  screenPage = BT_SCREEN_PIN;
  lastBleState = BLE_STATE_IDLE;
  lastUpdateTime = 0;
  showHex = false;

  drawPinScreen();
  screenMutator->drawOptionLabel("^ Back");
}

void BtConfigRenderer::renderLoop() {
  bleManager.loop();

  uint8_t currentState = bleManager.getState();

  // Transition: pairing (passkey) complete -> show the AES key QR.
  // Previously the key was shown as soon as *any* device connected, before
  // pairing, so an unauthenticated connection put the key on screen.
  if (currentState == BLE_STATE_AUTHENTICATED &&
      lastBleState < BLE_STATE_AUTHENTICATED) {
    screenPage = BT_SCREEN_QR;
    tft->fillScreen(TFT_BLACK);
    drawQrScreen();
    screenMutator->drawOptionLabel("^ Back");
  }

  // Transition: encrypted channel established -> show active
  if (currentState == BLE_STATE_ENCRYPTED &&
      lastBleState < BLE_STATE_ENCRYPTED) {
    screenPage = BT_SCREEN_ACTIVE;
    tft->fillScreen(TFT_BLACK);
    drawActiveScreen();
    screenMutator->drawOptionLabel("^ Back");
  }

  // Transition: disconnected -> back to PIN
  if (currentState == BLE_STATE_ADVERTISING &&
      lastBleState >= BLE_STATE_CONNECTED) {
    screenPage = BT_SCREEN_PIN;
    tft->fillScreen(TFT_BLACK);
    // Key rotation on disconnect is handled inside BleConfigManager.
    showHex = false;
    drawPinScreen();
    screenMutator->drawOptionLabel("^ Back");
  }

  // Periodic status update on active screen
  if (screenPage == BT_SCREEN_ACTIVE && millis() - lastUpdateTime > 2000) {
    lastUpdateTime = millis();
    tft->setTextSize(1);
    tft->setTextDatum(TL_DATUM);
    tft->setTextColor(TFT_GREEN);
    tft->fillRect(8, 56, 152, 20, TFT_BLACK);
    tft->drawString("Secrets: " + String(secretManager.getSecretCount()), 8,
                    56);
    tft->setTextColor(TFT_DARKGREY);
    tft->drawString(String(millis() / 1000) + "s uptime", 8, 68);
  }

  lastBleState = currentState;
}

void BtConfigRenderer::handleTopButton() {
  bleManager.stop();
  secretManager.end();
  screenMutator->setState(MENU_STATE_MAIN);
}

void BtConfigRenderer::handleBottomButton() {
  if (screenPage != BT_SCREEN_QR) return;
  showHex = !showHex;
  tft->fillScreen(TFT_BLACK);
  if (showHex) {
    drawHexScreen();
  } else {
    drawQrScreen();
  }
  screenMutator->drawOptionLabel("^ Back");
}

void BtConfigRenderer::handleBottomButtonLong() {}

void BtConfigRenderer::drawPinScreen() {
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(TFT_GREEN);
  tft->setTextSize(1);
  tft->drawString("BT Config Mode", 8, 4);

  tft->setTextColor(TFT_CYAN);
  tft->setTextSize(1);
  tft->drawString("Pairing PIN:", 8, 20);

  // Draw PIN large
  tft->setTextSize(3);
  tft->setTextColor(TFT_YELLOW);
  char pinStr[7];
  snprintf(pinStr, sizeof(pinStr), "%06lu", bleManager.getPin());
  tft->drawString(pinStr, 8, 34);

  tft->setTextSize(1);
  tft->setTextColor(TFT_DARKGREY);
  tft->drawString("Enter PIN when pairing", 8, 64);
}

void BtConfigRenderer::drawQrScreen() {
  // Get AES key as hex string (32 chars, fits in version 2 or 3 ECC-L)
  char hexKey[33];
  bleManager.getCrypto()->keyToHex(hexKey);

  // Generate packed 1-bit bitmap. Request 74px so it scales cleanly:
  //   v3: actualW=37 (29-module + 8 quiet zone), S=2 -> 74px output
  //   v2: actualW=33 (25-module + 8 quiet zone), S=2 -> 66px output
  int16_t qrSize = 0;
  uint8_t* bmp = QrCode::generate(hexKey, 3, 74, &qrSize);
  if (!bmp) return;

  int qrOriginX = (160 - qrSize) / 2;
  int qrOriginY = (80 - qrSize) / 2;

  // drawBitmap renders the packed 1-bit buffer: 1=black, 0=white background
  tft->drawXBitmap(qrOriginX, qrOriginY, bmp, qrSize, qrSize, TFT_BLACK,
                   TFT_WHITE);
  free(bmp);

  // Small label at top-left corner
  tft->setTextSize(1);
  tft->setTextColor(TFT_PINK);
  tft->setTextDatum(TL_DATUM);
  tft->drawString("Scan", 2, 0);
  tft->drawString("Key", 2, 10);
}

void BtConfigRenderer::drawHexScreen() {
  char hexKey[33];
  bleManager.getCrypto()->keyToHex(hexKey);

  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(TFT_PINK);
  tft->setTextSize(1);
  tft->drawString("Key (hex):", 8, 2);

  // Draw key as 4 rows of 8 hex chars at size 2 (fits 160px wide, 80px tall)
  // Size 2 = ~12px per char, 8 chars = 96px; centre at x=(160-96)/2=32
  tft->setTextSize(2);
  tft->setTextColor(TFT_CYAN);
  char row[9];
  for (int i = 0; i < 4; i++) {
    memcpy(row, hexKey + i * 8, 8);
    row[8] = '\0';
    tft->drawString(row, 32, 14 + i * 17);
  }
}

void BtConfigRenderer::drawActiveScreen() {
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(TFT_GREEN);
  tft->setTextSize(1);
  tft->drawString("BT Config Active", 8, 4);

  tft->setTextSize(2);
  tft->setTextColor(TFT_CYAN);
  tft->drawString("Encrypted", 8, 20);

  tft->setTextSize(1);
  tft->setTextColor(TFT_GREEN);
  tft->drawString("Channel established", 8, 42);

  tft->setTextColor(TFT_GREEN);
  tft->drawString("Secrets: " + String(secretManager.getSecretCount()), 8, 56);

  lastUpdateTime = millis();
}