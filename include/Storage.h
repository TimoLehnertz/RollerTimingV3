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

#define STORAGE_CHECK 1234

Preferences preferences;

void writePreferences() {
  Serial.println("Writing to preferences");
  preferences.putDouble("startDist", distFromStartInput->getValue());
  preferences.putDouble("minDelay", minDelayInput->getValue());
  preferences.putDouble("isDisplay", isDisplay());
  preferences.putDouble("brightness", displayBrightnessInput->getValue());
  preferences.putDouble("dispLapTime", displayTimeInput->getValue());
  preferences.putBool("isMaster", isMasterCB->isChecked());
}

void readPreferences() {
  Serial.println("Reading from preferences");
  distFromStartInput->setValue(preferences.getDouble("startDist"));
  minDelayInput->setValue(preferences.getDouble("minDelay"));
  isDisplayCheckbox->setChecked(preferences.getDouble("isDisplay"));
  displayBrightnessInput->setValue(preferences.getDouble("brightness"));
  displayTimeInput->setValue(preferences.getDouble("dispLapTime"));
  isMasterCB->setChecked(preferences.getBool("isMaster"));
}

void resetAllSettings() {
  distFromStartInput->setValue(30);
  minDelayInput->setValue(5);
  isDisplayCheckbox->setChecked(false);
  displayBrightnessInput->setValue(30);
  displayTimeInput->setValue(3);
  isMasterCB->setChecked(false);
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