#pragma once

#ifndef IInputHandler_h
#define IInputHandler_h

class IInputHandler {
 public:
  virtual void renderInit();
  virtual void handleTopButton();
  virtual void handleBottomButton();
  virtual void handleBottomButtonLong();
};

#endif
