#pragma once

#ifndef TimeManager_h
#define TimeManager_h

#include "Arduino.h"
#include "time.h"

#define NTP_SERVER      "time1.google.com"
#define GMT_OFFSET_SECS 28800
#define MINUTE 60
#define HOUR 60 * 60
#define DAY 24 * 60 * 60
#define MONTH

class TimeData {
  private:
    tm time;
    bool hasInited;

  public:
    TimeData();
    TimeData(tm timeDat);
    TimeData(const TimeData &other);

    tm toTime();
    bool isValid();
    long unix();
};

class TimeManager {
  private:
    bool isTimeSynced;
    static TimeManager* instance;

  public:
    TimeManager();
    bool syncTime();
    TimeData getTime();
    bool hasSynced();
    static TimeManager* getInstance();
};

#endif
