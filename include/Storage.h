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
#include <GuiLogic.h>

#define STORAGE_CHECK 1234

Preferences preferences;

void isDisplayChanged();

void writePreferences() {
  Serial.println("Writing to preferences");
  preferences.putDouble("startDist", distFromStartInput->getValue());
  preferences.putDouble("minDelay", minDelayInput->getValue());
  preferences.putDouble("brightness", displayBrightnessInput->getValue());
  preferences.putDouble("dispLapTime", displayTimeInput->getValue());
  preferences.putInt("isDisplay", isDisplaySelect->getValue());
  preferences.putBool("isMaster", isMasterCB->isChecked());
  preferences.putInt("trainingsMode", trainingsModeSelect->getValue());
  preferences.putInt("stationType", stationTypeSelect->getValue());
  preferences.putInt("uid", uidInput->getValue());
}

void readPreferences() {
  Serial.println("Reading from preferences");
  distFromStartInput->setValue(preferences.getDouble("startDist"));
  minDelayInput->setValue(preferences.getDouble("minDelay"));
  displayBrightnessInput->setValue(preferences.getDouble("brightness"));
  displayTimeInput->setValue(preferences.getDouble("dispLapTime"));
  isMasterCB->setChecked(preferences.getBool("isMaster"));
  isDisplaySelect->setValue(preferences.getInt("isDisplay"));
  trainingsModeSelect->setValue(preferences.getInt("trainingsMode"));
  stationTypeSelect->setValue(preferences.getInt("stationType"));
  uidInput->setValue(preferences.getInt("uid"));
}

/**
 * @note Blocking for more than 100ms 
 */
void resetAllSettings() {
  distFromStartInput->setValue(10);
  minDelayInput->setValue(5);
  displayBrightnessInput->setValue(30);
  displayTimeInput->setValue(3);
  trainingsModeSelect->setValue(TRAININGS_MODE_NORMAL);
  stationTypeSelect->setValue(STATION_TRIGGER_TYPE_START_FINISH);
  // determine if this is a display or laser by checking if PIN_LASER is floating
  // u8_t floatingCount = 0;
  // for (size_t i = 0; i < 100; i++) {
  //   uint16_t laser = analogRead(PIN_LASER);
  //   Serial.println(laser);
  //   if(laser > 30 && laser < 4000) {
  //     floatingCount++;
  //   }
  //   delay(1);
  // }
  // Serial.printf("floatingCount: %i\n", floatingCount);
  // bool displayStation = floatingCount > 10; // is floating
  bool displayStation = true; // likely more often updated
  isDisplaySelect->setValue(displayStation);
  isMasterCB->setChecked(displayStation);
  isDisplayChanged();
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