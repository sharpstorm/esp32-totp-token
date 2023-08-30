#include "TimeManager.h"

TimeData::TimeData(): hasInited(false) {}
TimeData::TimeData(tm timeDat): hasInited(true), time(timeDat) {}
TimeData::TimeData(const TimeData &other): hasInited(other.hasInited), time(other.time) {}

tm TimeData::toTime() {
  return time;
}

bool TimeData::isValid() {
  return hasInited;
}

long TimeData::unix() {
  return (long) mktime(&time);
}

TimeManager* TimeManager::instance = nullptr;
TimeManager::TimeManager(): isTimeSynced(false) {}

bool TimeManager::syncTime() {
  configTime(GMT_OFFSET_SECS, 0, NTP_SERVER);
  struct tm timeInfo;
  if(!getLocalTime(&timeInfo)){
    return false;
  }

  isTimeSynced = true;
  return true;
}

TimeData TimeManager::getTime() {
  if (!isTimeSynced) {
    return TimeData();
  }

  struct tm timeInfo;
  if (!getLocalTime(&timeInfo)) {
    return TimeData();
  }

  return TimeData(timeInfo);
}

TimeManager* TimeManager:: getInstance() {
  if (instance == nullptr) {
    instance = new TimeManager();
  }

  return instance;
}

bool TimeManager::hasSynced() {
  return isTimeSynced;
}
