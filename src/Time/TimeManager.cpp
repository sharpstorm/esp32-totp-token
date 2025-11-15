#include "TimeManager.h"

TimeManager* TimeManager::instance = nullptr;
TimeManager::TimeManager() {}

static void setTimeZone(long offset, int daylight) {
  char cst[17] = {0};
  char cdt[17] = "DST";
  char tz[33] = {0};

  if (offset % 3600) {
    sprintf(cst, "UTC%ld:%02u:%02u", offset / 3600, abs((offset % 3600) / 60),
            abs(offset % 60));
  } else {
    sprintf(cst, "UTC%ld", offset / 3600);
  }
  if (daylight != 3600) {
    long tz_dst = offset - daylight;
    if (tz_dst % 3600) {
      sprintf(cdt, "DST%ld:%02u:%02u", tz_dst / 3600, abs((tz_dst % 3600) / 60),
              abs(tz_dst % 60));
    } else {
      sprintf(cdt, "DST%ld", tz_dst / 3600);
    }
  }
  sprintf(tz, "%s%s", cst, cdt);
  setenv("TZ", tz, 1);
  tzset();
}

bool TimeManager::syncTime() {
  configTime(GMT_OFFSET_SECS, 0, NTP_SERVER);
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo)) {
    return false;
  }

  RTC_DateTypeDef date;
  date.Date = timeInfo.tm_mday;
  date.Month = timeInfo.tm_mon + 1;
  date.Year = timeInfo.tm_year + 1900;
  date.WeekDay = timeInfo.tm_wday;

  RTC_TimeTypeDef time;
  time.Hours = timeInfo.tm_hour;
  time.Minutes = timeInfo.tm_min;
  time.Seconds = timeInfo.tm_sec;

  M5.Rtc.SetTime(&time);
  M5.Rtc.SetData(&date);

  return true;
}

TimeData TimeManager::getTime() {
  RTC_TimeTypeDef time;
  RTC_DateTypeDef date;

  M5.Rtc.GetTime(&time);
  M5.Rtc.GetData(&date);

  return TimeData(&date, &time);
}

TimeManager* TimeManager::getInstance() {
  setTimeZone(-GMT_OFFSET_SECS, 0);
  if (instance == nullptr) {
    instance = new TimeManager();
  }

  return instance;
}
