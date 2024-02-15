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

#define STORAGE_CHECK 20004 // count up by one if any changes were made in this file

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
  preferences.putString("wifiSSID", uploadWifiSSID);
  preferences.putString("wifiPassword", uploadWifiPassword);
  preferences.putString("username", username);
  // preferences.putBool("wifiOn", wifiEnabledCB->isChecked());
  preferences.putString("APSsid", APSsid);
  preferences.putString("APPassword", APPassword);
  preferences.putInt("lapDisplayType", lapDisplayTypeSelect->getValue());
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
  uploadWifiSSID = preferences.getString("wifiSSID");
  uploadWifiPassword = preferences.getString("wifiPassword");
  username = preferences.getString("username");
  lapDisplayTypeSelect->setValue(preferences.getInt("lapDisplayType"));
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
  distFromStartInput->setValue(30);
  minDelayInput->setValue(1);
  displayBrightnessInput->setValue(10);
  displayTimeInput->setValue(7);
  trainingsModeSelect->setValue(TRAININGS_MODE_NORMAL);
  stationTypeSelect->setValue(STATION_TRIGGER_TYPE_START_FINISH);
  cloudUploadEnabled->setChecked(false);
  fontSizeSelect->setValue(0);
  uploadWifiSSID = "";
  uploadWifiPassword = "";
  username = "";
  APSsid = APSSID_DISPLAY_DEFAULT;
  APPassword = APPASSWORD_DEFAULT;
  lapDisplayTypeSelect->setValue(0); // lap time mode
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
  // isDisplaySelect->setValue(false); // likely more often updated
  // isDisplayChanged();
}

void factoryReset() {
  Serial.println("Resetting");
  uiManager.handle(true);
  int isDisplay = preferences.getInt("isDisplay");
  resetAllSettings();
  writePreferences();
  preferences.putInt("isDisplay", isDisplay);
  uiManager.popup("Factory reset done");
  uiManager.handle(true);
  delay(1000);
  uiManager.popup("Rebooting...");
  uiManager.handle(true);
  delay(1000);
  ESP.restart();
}

void beginPreferences() {
  preferences.begin("rr-timing", false);
  if(preferences.getInt("check") == STORAGE_CHECK) {
    readPreferences();
  } else {
    preferences.putInt("check", STORAGE_CHECK);
    factoryReset();
  }
}