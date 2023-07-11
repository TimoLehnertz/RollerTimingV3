#pragma once

#include <Global.h>

void RotaryChanged() {
  const unsigned int state = Rotary.GetState();
  if (state & DIR_CW) Counter++;
  if (state & DIR_CCW) Counter--;
}

void handleRotary() {
  static bool lastBtn = false;
  while(LastCount < Counter) {
    LastCount++;
    uiManager.triggerInputEvent(INPUT_EVENT_SCROLL_DOWN);
  }
  while(LastCount > Counter) {
    LastCount--;
    uiManager.triggerInputEvent(INPUT_EVENT_SCROLL_UP);
  }
  bool rotaryPressed = Rotary.isButtonDown();
  if(lastBtn != rotaryPressed) {
  if(rotaryPressed) {
      uiManager.triggerInputEvent(INPUT_EVENT_MOUSE_DOWN);
  } else {
      uiManager.triggerInputEvent(INPUT_EVENT_MOUSE_UP);
  }
  lastBtn = rotaryPressed;
  }
}