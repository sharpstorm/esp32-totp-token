#include "TimeData.h"

TimeData::TimeData() : hasInited(false) {}

TimeData::TimeData(RTC_DateTypeDef* date, RTC_TimeTypeDef* time)
    : hasInited(true),
      year(date->Year),
      month(date->Month),
      day(date->Date),
      hour(time->Hours),
      minute(time->Minutes),
      second(time->Seconds) {}

TimeData::TimeData(const TimeData& other)
    : hasInited(other.hasInited),
      year(other.year),
      month(other.month),
      day(other.day),
      hour(other.hour),
      minute(other.minute),
      second(other.second) {}

String TimeData::toDateString() {
  char buf[11];
  // dd/mm/yyyy
  sprintf(buf, "%02d/%02d/%04d", day, month, year);
  buf[10] = 0;  // End of string NUL character
  return String(buf);
}

String TimeData::toTimeString() {
  char buf[9];
  // hh:mm:ss
  sprintf(buf, "%02d:%02d:%02d", hour, minute, second);
  buf[8] = 0;  // End of string NUL character
  return String(buf);
}

bool TimeData::isValid() { return hasInited; }

long TimeData::unix() {
  struct tm time = {.tm_sec = second,
                    .tm_min = minute,
                    .tm_hour = hour,
                    .tm_mday = day,
                    .tm_mon = month - 1,
                    .tm_year = year - 1900};

  return (long)mktime(&time);
}
