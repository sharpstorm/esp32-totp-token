#pragma once

#ifndef AppUtil_h
#define AppUtil_h

#include <M5StickC.h>

#include "Time/TimeManager.h"
#include "time.h"

class RendererUtil {
 public:
  static void drawLocalTime(M5Display* tft, int x, int y) {
    TimeData timeDat = TimeManager::getInstance()->getTime();
    if (!timeDat.isValid()) {
      return;
    }

    tft->fillRect(x, y, 180, 24, TFT_BLACK);
    tft->drawString(timeDat.toDateString(), x, y);
    tft->drawString(timeDat.toTimeString(), x, y + 12);
  }

  static void drawUnixTime(M5Display* tft, int x, int y) {
    TimeData timeDat = TimeManager::getInstance()->getTime();
    if (!timeDat.isValid()) {
      return;
    }

    tft->fillRect(x, y, 180, 24, TFT_BLACK);
    tft->drawString(String(timeDat.unix()), x, y);
  }
};

#endif
