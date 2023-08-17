#include <TFT_eSPI.h>
#include <SPI.h>
#include "WiFi.h"
#include <Wire.h>
#include "Button2.h"
#include "time.h"
#include "ScreenManager.h"
#include "Util.h"

#include "driver/rtc_io.h"

#define ADC_EN              14  //ADC_EN is the ADC detection enable port
#define BUTTON_1            35
#define BUTTON_2            0

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library
Button2 rightBtn(BUTTON_1);
Button2 leftBtn(BUTTON_2);
ScreenManager screenManager(&tft);

Util::TimeoutFunction Util::espDelay = [](int ms){
  esp_sleep_enable_timer_wakeup(ms * 1000);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  esp_light_sleep_start();
};

void button_init() {
  rightBtn.setPressedHandler([](Button2 & b) {
    screenManager.handleTopButton();
  });

  leftBtn.setPressedHandler([](Button2 & b) {
    screenManager.handleBottomButton();
  });

  leftBtn.setLongClickHandler([](Button2 & b) {
    screenManager.handleBottomButtonLong();
  });
}

void button_loop() {
  leftBtn.loop();
  rightBtn.loop();
}

void setup() {
  // Init
  setCpuFrequencyMhz(80);
  WiFi.setSleep(true);
  WiFi.mode(WIFI_OFF);
  btStop();
  Serial.begin(115200);
  Serial.println("Token Debug Start");

  /*
  ADC_EN is the ADC detection enable port
  If the USB port is used for power supply, it is turned on by default.
  If it is powered by battery, it needs to be set to high level
  */
  pinMode(ADC_EN, OUTPUT);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.setSwapBytes(true);
 
  button_init();
  screenManager.setState(MENU_STATE_MAIN);
}

void loop() {
  if (screenManager.isGlobalDirty()) {
    tft.fillScreen(TFT_BLACK);
    screenManager.resetGlobalDirty();
    screenManager.resetDrawConfig();
    screenManager.renderInit();
  }

  screenManager.renderLoop();
  button_loop();
}
