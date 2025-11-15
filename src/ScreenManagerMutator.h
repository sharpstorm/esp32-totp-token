#pragma once

#ifndef ScreenManagerMutator_h
#define ScreenManagerMutator_h

#define MENU_STATE_MAIN 1
#define MENU_STATE_SYNC 2
#define MENU_STATE_OTP 3
#define MENU_STATE_CONFIG 4
#define MENU_STATE_INFO 5

class ScreenManagerMutator {
 public:
  virtual void setState(int state);
  virtual void setGlobalDirty();
  virtual void drawOptionLabel(const char* top);
  virtual void resetDrawConfig();
  virtual void saveDrawConfig();
  virtual void restoreDrawConfig();
};

#endif
