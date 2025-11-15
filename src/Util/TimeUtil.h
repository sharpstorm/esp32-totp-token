#pragma once

#include <M5StickC.h>

class TimeUtil {
 public:
  typedef void (*TimeoutFunction)(int ms);
  static TimeoutFunction espDelay;
};
