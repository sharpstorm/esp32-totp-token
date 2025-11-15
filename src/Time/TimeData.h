#pragma once

#ifndef TimeData_h
#define TimeData_h

#include <M5StickC.h>

#include "time.h"

struct TimeData {
 private:
  bool hasInited;

 public:
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
  uint8_t day;
  uint8_t month;
  uint16_t year;

  TimeData();
  TimeData(RTC_DateTypeDef* date, RTC_TimeTypeDef* time);
  TimeData(const TimeData& other);

  String toDateString();
  String toTimeString();
  bool isValid();
  long unix();
};

#endif
