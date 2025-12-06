#include <M5StickC.h>
#include <SPI.h>
#include <Wire.h>

#include "SPIFFS.h"
#include "ScreenManager.h"
#include "Wifi/WifiManager.h"
#include "time.h"

ScreenManager screenManager(&M5.Lcd);

void button_loop() {
  if (M5.BtnB.wasPressed()) {
    screenManager.handleTopButton();
  } else if (M5.BtnA.pressedFor(1000)) {
    screenManager.handleBottomButtonLong();
  } else if (M5.BtnA.wasPressed()) {
    screenManager.handleBottomButton();
  }
}

void setup() {
  // Init
  setCpuFrequencyMhz(80);
  wifiManager.disconnectWifi();
  btStop();
  SPIFFS.begin(true);
  M5.begin();
  M5.Lcd.setRotation(1);
  screenManager.setState(MENU_STATE_OTP);
}

void loop() {
  if (screenManager.isGlobalDirty()) {
    M5.Lcd.fillScreen(TFT_BLACK);
    screenManager.resetGlobalDirty();
    screenManager.resetDrawConfig();
    screenManager.renderInit();
  }

  M5.update();
  button_loop();
  screenManager.renderLoop();
}
