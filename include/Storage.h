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
#include <WiFiLogic.h>

#define STORAGE_CHECK 201 // version + garbaage

Preferences preferences;

void isDisplayChanged();
// void wifiEnabledChanged();

void writePreferences() {
  Serial.println("Writing to preferences");
  // preferences.putDouble("startDist", distFromStartInput->getValue());
  preferences.putDouble("minDelay", minDelayInput->getValue());
  preferences.putDouble("brightness", displayBrightnessInput->getValue());
  preferences.putDouble("dispLapTime", displayTimeInput->getValue());
  preferences.putInt("isDisplay", isDisplaySelect->getValue());
  preferences.putInt("trainingsMode", trainingsModeSelect->getValue());
  // preferences.putInt("stationType", stationTypeSelect->getValue());
  preferences.putBool("uploadEnabled", cloudUploadEnabled->isChecked());
  preferences.putInt("fontSize", fontSizeSelect->getValue());
  preferences.putString("wifiSSID", wifiSSID);
  preferences.putString("wifiPassword", wifiPassword);
  preferences.putString("username", username);
  // preferences.putBool("wifiOn", wifiEnabledCB->isChecked());
  if(isDisplaySelect->getValue()) { // is display
    preferences.putString("APSsid", APSsid);
    preferences.putString("APPassword", APPassword);
  }
}

void readPreferences() {
  Serial.println("Reading from preferences");
  // distFromStartInput->setValue(preferences.getDouble("startDist"));
  minDelayInput->setValue(preferences.getDouble("minDelay"));
  displayBrightnessInput->setValue(preferences.getDouble("brightness"));
  displayTimeInput->setValue(preferences.getDouble("dispLapTime"));
  isDisplaySelect->setValue(preferences.getInt("isDisplay"));
  trainingsModeSelect->setValue(preferences.getInt("trainingsMode"));
  // stationTypeSelect->setValue(preferences.getInt("stationType"));
  cloudUploadEnabled->setChecked(preferences.getBool("uploadEnabled"));
  fontSizeSelect->setValue(preferences.getInt("fontSize"));
  wifiSSID = preferences.getString("wifiSSID");
  wifiPassword = preferences.getString("wifiPassword");
  username = preferences.getString("username");
  // if(isDisplaySelect->getValue()) { // only activate wifi if this is a display
  //   wifiEnabledCB->setChecked(preferences.getBool("wifiOn"), false);
  //   wifiEnabledChanged();
  // }
  if(isDisplaySelect->getValue()) { // is display
    APSsid = preferences.getString("APSsid");
    APPassword = preferences.getString("APPassword");
  } // otherwise ssid and password will be set by defaults
  wiFiCredentialsChanged();
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
  cloudUploadEnabled->setChecked(false);
  fontSizeSelect->setValue(0);
  wifiSSID = "";
  wifiPassword = "";
  username = "";
  // wifiEnabledCB->setChecked(true);
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
  isDisplaySelect->setValue(false); // likely more often updated
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