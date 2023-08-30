#include "RendererInfo.h"

#include "Util.h"
#include "time.h"
#include <esp_adc_cal.h>

#define ADC_EN              14  //ADC_EN is the ADC detection enable port
#define ADC_PIN             34

InfoRenderer::InfoRenderer(TFT_eSPI *tftRef, ScreenManagerMutator *screenMutator):
  tft(tftRef), screenMutator(screenMutator) {}

void InfoRenderer::renderInit() {
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(TFT_GREEN);
  tft->setTextSize(2);
  tft->drawString("Info Mode", 16, 16);
  
  screenMutator->drawButtonLabels("Page >", "Back >");
  getRefVoltage();
  timeStamp = 0;
  page = 0;
  secretManager.start();
}

void InfoRenderer::renderLoop() {
  if (millis() - timeStamp > 1000) {
    timeStamp = millis();
    tft->fillRect(16, 52, 224, 60, TFT_BLACK);

    switch (page) {
      case INFO_MENU_VOLTAGE:
        renderVoltage();
        break;
      case INFO_MENU_TIME:
        renderTime();
        break;
      case INFO_MENU_UNIX_TIME:
        renderUnixTime();
        break;
      case INFO_MENU_SECRETS:
        renderSecrets();
        break;
      default:
        break;
    }
  }
}

void InfoRenderer::renderVoltage() {
  uint16_t v = analogRead(ADC_PIN);
  float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);

  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(TFT_PINK);
  tft->drawString("Voltage:", 16, 52);
  tft->setTextColor(TFT_CYAN);
  tft->drawString(String(battery_voltage) + " V", 16, 72);
}

void InfoRenderer::renderTime() {
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(TFT_PINK);
  tft->drawString("Time:", 16, 52);
  tft->setTextColor(TFT_CYAN);
  Util::drawLocalTime(tft, 16, 72);
}

void InfoRenderer::renderUnixTime() {
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(TFT_PINK);
  tft->drawString("Unix Time:", 16, 52);
  tft->setTextColor(TFT_CYAN);
  Util::drawUnixTime(tft, 16, 72);
}

void InfoRenderer::renderSecrets() {
  tft->setTextDatum(TL_DATUM);
  tft->setTextColor(TFT_PINK);
  tft->drawString("Secret Count:", 16, 52);
  tft->drawString(String(secretManager.getSecretCount()), 180, 52);
}

void InfoRenderer::handleTopButton() {
  page = (page + 1) % 4;
}

void InfoRenderer::handleBottomButton() {
  digitalWrite(ADC_EN, LOW);
  secretManager.end();
  screenMutator->setState(MENU_STATE_MAIN);
}

void InfoRenderer::handleBottomButtonLong() {
}

void InfoRenderer::getRefVoltage() {
  digitalWrite(ADC_EN, HIGH);

  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);    //Check type of calibration value used to characterize ADC
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
      Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
      vref = adc_chars.vref;
  } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
      Serial.printf("Two Point --> coeff_a:%umV coeff_b:%umV\n", adc_chars.coeff_a, adc_chars.coeff_b);
  } else {
      Serial.println("Default Vref: 1100mV");
  }
}
