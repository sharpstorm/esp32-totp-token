#pragma once

#ifndef TimeManager_h
#define TimeManager_h

#include <M5StickC.h>

#include "TimeData.h"
#include "time.h"

#define NTP_SERVER "time1.google.com"
#define GMT_OFFSET_SECS 28800
#define MINUTE 60
#define HOUR 60 * 60
#define DAY 24 * 60 * 60
#define MONTH

class TimeManager {
 private:
  static TimeManager* instance;

 public:
  TimeManager();
  bool syncTime();
  TimeData getTime();
  static TimeManager* getInstance();
};

#endif
