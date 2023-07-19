/**
 * @file Storage.h
 * @author Timo Lehnertz
 * @brief Contains all code related to storing data outside the ram
 * @version 0.1
 * @date 2023-07-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once

#include <Preferences.h>
#include <Global.h>

#define STORAGE_CHECK 123456

Preferences preferences;

void writePreferences() {
  Serial.println("Writing to preferences");
  preferences.putDouble("startDist", distFromStartInput->getValue());
  preferences.putDouble("minDelay", minDelayInput->getValue());
  preferences.putBool("display", displayStation);
  preferences.putDouble("brightness", displayBrightnessInput->getValue());
  preferences.putDouble("dispLapTime", displayTimeInput->getValue());
  preferences.putBool("isMaster", isMasterCB->isChecked());
}

void readPreferences() {
  Serial.println("Reading from preferences");
  distFromStartInput->setValue(preferences.getDouble("startDist"));
  minDelayInput->setValue(preferences.getDouble("minDelay"));
  displayBrightnessInput->setValue(preferences.getDouble("brightness"));
  displayTimeInput->setValue(preferences.getDouble("dispLapTime"));
  isMasterCB->setChecked(preferences.getBool("isMaster"));
  displayStation = preferences.getBool("display");
}

/**
 * @note Blocking for more than 100ms 
 */
void resetAllSettings() {
  distFromStartInput->setValue(10);
  minDelayInput->setValue(5);
  displayBrightnessInput->setValue(30);
  displayTimeInput->setValue(3);
  // determine if this is a display or laser by checking if PIN_LASER is floating
  u8_t floatingCount = 0;
  for (size_t i = 0; i < 100; i++) {
    uint16_t laser = analogRead(PIN_LASER);
    Serial.println(laser);
    if(laser > 30 && laser < 4000) {
      floatingCount++;
    }
    delay(1);
  }
  Serial.printf("floatingCount: %i\n", floatingCount);
  displayStation = floatingCount > 10; // is floating
  isMasterCB->setChecked(displayStation);
}

void factoryReset() {
  Serial.println("Factory resetting");
  resetAllSettings();
  writePreferences();
  uiManager.popup("Factory reset done");
}

void beginPreferences() {
  preferences.begin("rr-timing", false);
  if(preferences.getInt("check") == STORAGE_CHECK) {
    readPreferences();
  } else {
    factoryReset();
    preferences.putInt("check", STORAGE_CHECK);
  }
}