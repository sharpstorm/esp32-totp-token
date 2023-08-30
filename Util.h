#pragma once

#ifndef AppUtil_h
#define AppUtil_h

#include <TFT_eSPI.h>
#include "time.h"
#include "src/Time/TimeManager.h"

class Util {
  public:
    typedef void (*TimeoutFunction) (int ms);
    static TimeoutFunction espDelay;

    static void drawLocalTime(TFT_eSPI *tft, int x, int y) {
      TimeData timeDat = TimeManager::getInstance()->getTime();
      if (!timeDat.isValid()) {
        return;
      }

      tm timeinfo = timeDat.toTime();
    
      char buffer1[25];
      char buffer2[25];
      strftime(buffer1, sizeof(buffer1), "%a, %b %d %Y", &timeinfo);
      strftime(buffer2, sizeof(buffer2), "%I : %M : %S %p", &timeinfo);
    
      tft->fillRect(x, y, 200, 40, TFT_BLACK);
      tft->drawString(buffer1, x, y);
      tft->drawString(buffer2, x, y + 20);
    }

    static void drawUnixTime(TFT_eSPI *tft, int x, int y) {
      TimeData timeDat = TimeManager::getInstance()->getTime();
      if (!timeDat.isValid()) {
        return;
      }

      tm timeinfo = timeDat.toTime();
    
      tft->fillRect(x, y, 200, 40, TFT_BLACK);
      tft->drawString(String(timeDat.unix()), x, y);
    }
};

#endif
