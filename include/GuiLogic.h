/**
 * @file GuiLogic.h
 * @author Timo Lehnertz
 * @brief File for creating and managing everything related to Displays
 * @version 0.1
 * @date 2023-07-11
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once
#include <Arduino.h>
#include <definitions.h>
#include <Storage.h>
#include <WiFiLogic.h>
#include <startgun.h>
#include <SPIFFSLogic.h>
#include <WiFiLogic.h>

#define POWER_SAVING_MODE_OFF 0
#define POWER_SAVING_MODE_MEDIUM 1
#define POWER_SAVING_MODE_HIGH 2

#define MASTER_FRAMES 3
#define SLAVE_FRAMES 3

// TextItem* connectionItems[MAX_CONNECTIONS];
// char* connectionItemsTexts[MAX_CONNECTIONS];

// size_t connectionCount = 0;

void beginLEDDisplay();
void trainingsModeChanged();

int getLEDCount() {
  return isDisplaySelect->getValue() ? NUM_LEDS_DISPLAY : NUM_LEDS_LASER;
}

uint16_t pixelConverter(uint16_t x, uint16_t y) {
  y = 7 - y;
  return (8 * 31) - x * 8 + (x % 2 == 0 ? (7 - y) : y);
}

void showDebugChanged() {
  debugSubMenu->setHidden(!showAdvancedCB->isChecked());
  advancedText->setHidden(!showAdvancedCB->isChecked());
  isDisplaySelect->setHidden(!showAdvancedCB->isChecked());
}

void simpleInputChanged() {
  writePreferences();
}

void reboot() {
  ESP.restart();
}

void wiFiCredentialsChanged() {
  static char* wifiSSIDStr = new char[40];
  static char* wifiPasswdStr = new char[40];
  sprintf(wifiSSIDStr, "SSID: %s", APSsid.c_str());
  sprintf(wifiPasswdStr, "Password: %s", APPassword.c_str());
  wifiSSIDText->setText(wifiSSIDStr);
  wifiPasswdText->setText(wifiPasswdStr);
}

void initStationDisplay() {
  displayBrightnessInput->setHidden(!isDisplaySelect->getValue());
  displayTimeInput->setHidden(!isDisplaySelect->getValue());
  fontSizeSelect->setHidden(!isDisplaySelect->getValue());
  lapDisplayTypeSelect->setHidden(!isDisplaySelect->getValue());
  cloudUploadEnabled->setHidden(!isDisplaySelect->getValue());
  // only on lasers
  stationTypeSelect->setHidden(isDisplaySelect->getValue());
  minDelayInput->setHidden(isDisplaySelect->getValue());
  // wifiEnabledCB->setHidden(isDisplaySelect->getValue());
  // master slave
  if(isDisplaySelect->getValue()) { // now I am a display
    spiffsLogic.startNewSession();
    uiManager.begin(overlayCallbacks, overlaysCount, frameSections, MASTER_FRAMES);
    trainingsModeChanged();
  } else { // now I am a station
    uiManager.begin(overlayCallbacks, overlaysCount, frameSections, SLAVE_FRAMES);
    targetTimeSubMenu->setHidden(true);
    // reset SSID and password
    APSsid = APSSID_STATION_DEFAULT;
    APPassword = APPASSWORD_DEFAULT;
  }
  wiFiCredentialsChanged();
  trainingsModeSelect->setHidden(!isDisplaySelect->getValue());
}

void isDisplayChanged() {
  writePreferences();
  uiManager.popup("Rebooting...");
  uiManager.handle(true);
  delay(1000);
  ESP.restart();
}

// void guiSetConnection(uint8_t address, int millimeters, uint8_t lq, uint8_t stationType) {
//   if(stationType == STATION_TRIGGER_TYPE_NONE) return;
//   if(address >= MAX_CONNECTIONS) return;
//   if(connectionItemsTexts[address] == nullptr) {
//     connectionItemsTexts[address] = new char[40];
//     connectionCount++;
//     playSoundNewConnection();
//   }
//   switch(stationType) {
//     case STATION_TRIGGER_TYPE_CHECKPOINT: {
//       sprintf(connectionItemsTexts[address], "Checkpoint %.1fm(Lq%i%)", millimeters / 1000.0, lq);
//       break;
//     }
//     case STATION_TRIGGER_TYPE_FINISH: {
//       sprintf(connectionItemsTexts[address], "Finish (Lq%i%)", lq);
//       break;
//     }
//     case STATION_TRIGGER_TYPE_START: {
//       sprintf(connectionItemsTexts[address], "Start (Lq%i%)", lq);
//       break;
//     }
//     case STATION_TRIGGER_TYPE_START_FINISH: {
//       sprintf(connectionItemsTexts[address], "Start + finish (Lq%i%)", lq);
//       break;
//     }
//     default: {
//       connectionItemsTexts[address][1] = '-';
//       connectionItemsTexts[address][0] = 0;
//       break;
//     }
//   }
//   if(connectionItems[address] == nullptr) {
//     connectionItems[address] = new TextItem(connectionItemsTexts[address], true);
//     connectionsMenuMaster->addItem(connectionItems[address]);
//   }
// }

// void guiRemoveConnection(uint8_t address) {
//   if(address >= MAX_CONNECTIONS) return;
//   if(connectionItems[address] != nullptr) {
//     connectionsMenuMaster->removeItem(connectionItems[address]);
//     delete connectionItems[address];
//     connectionItems[address] = nullptr;
//     connectionCount--;
//     playSoundLostConnection();
//   }
//   if(connectionItemsTexts[address] != nullptr) {
//     delete[] connectionItemsTexts[address];
//     connectionItemsTexts[address] = nullptr;
//   }
// }

void msOverlay(ScreenDisplay *display, DisplayUiState* state) {
  display->setColor(BLACK);
  display->fillRect(0, 0, 128, 13);
  display->setColor(WHITE);
  display->fillRect(7, 13, 114, 1);

  char strConnection[30];
  strConnection[0] = 0;
  if(!isDisplaySelect->getValue()) { // slave
    if(masterConnected) {
      if(slaveTriggers.getSize() > 0) {
        sprintf(strConnection, "Connected(%i qued)", slaveTriggers.getSize());
      } else {
        sprintf(strConnection, "Connected");
      }
    } else {
      sprintf(strConnection, "No connection");
    }
    display->setFont(ArialMT_Plain_10);
    display->setTextAlignment(TEXT_ALIGN_LEFT);
    display->drawString(0, 0, String(strConnection));
    // type of station
    display->setTextAlignment(TEXT_ALIGN_RIGHT);
    display->drawString(128, 0, String(stationTypeSelect->getSelectedShort()));
  } else {
    size_t lapsCount = spiffsLogic.getActiveTraining().getLapsCount();
    size_t bytesUsed = spiffsLogic.getBytesTotal();
    if(bytesUsed > 0) {
      size_t memUsed = float(spiffsLogic.getBytesUsed()) / float(bytesUsed) * 100;
      char memUsedStr[20];
      memUsedStr[0] = 0;
      sprintf(memUsedStr, "Free storage: %i%%", 100 - memUsed);
      display->setFont(ArialMT_Plain_10);
      display->setTextAlignment(TEXT_ALIGN_LEFT);
      display->drawString(0, 0, String(memUsedStr));
    }
  }
}

void drawFrameWiFi(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) {
  // draw an xbm image.
  display->drawXbm(x + 34, y + 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
}

void drawStartGun(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display->setFont(ArialMT_Plain_16);
  display->drawStringMaxWidth(x + 64, y + 32, 100, "Start gun");
  // display->drawXbm(x, y, START_GUN_WIDTH, START_GUN_HEIGHT, IMAGE_START_GUN);
}

void drawViewer(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display->setFont(ArialMT_Plain_16);
  display->drawStringMaxWidth(x + 64, y + 32, 100, "Viewer");
}

void drawSetup(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display->setFont(ArialMT_Plain_16);
  display->drawStringMaxWidth(x + 64, y + 32, 100, "Setup");
}

// void drawConnections(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) {
//   display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
//   display->setFont(ArialMT_Plain_16);
//   display->drawStringMaxWidth(x + 64, y + 32, 100, "Connections");
// }

void setMinDelayChanged() {
  setMaxDelay->setValue(max(setMaxDelay->getValue(), setMinDelay->getValue()));
}

void setMaxDelayChanged() {
  setMinDelay->setValue(min(setMaxDelay->getValue(), setMinDelay->getValue()));
}

void goMinDelayChanged() {
  goMaxDelay->setValue(max(goMaxDelay->getValue(), goMinDelay->getValue()));
}

void goMaxDelayChanged() {
  goMinDelay->setValue(min(goMaxDelay->getValue(), goMinDelay->getValue()));
}

void resetStartGun() {
  inPositionDelay->setValue(15);
  setMinDelay->setValue(3);
  setMaxDelay->setValue(5);
  goMinDelay->setValue(2);
  goMaxDelay->setValue(5);
}

void resetTarget() {
  targetTimeInput->setValue(300000);
  targetMetersInput->setValue(3000);
  targetMetersPerLapInput->setValue(200);
}

void startGunNowBtnPressed() {
  inPositionDelay->setValue(0);
  triggerStartGun();
}

void startGun15sBtnPressed() {
  inPositionDelay->setValue(15);
  triggerStartGun();
}

void cloudUploadChanged() {
  if(uploadWifiSSID.length() < 2) {
    uiManager.popup("Setup on website first!");
    cloudUploadEnabled->setChecked(false);
    return;
  }
  writePreferences();
}

uint16_t stopWatchLap = 0;

void startBtnPressed() {
  stopWatchLap = 0;
  Trigger trigger = Trigger { timeMs_t(millis()), 0, STATION_TRIGGER_TYPE_START };
  if(isDisplaySelect->getValue()) { // master
    masterTrigger(trigger);
  } else {
    slaveTrigger(millis(), STATION_TRIGGER_TYPE_START, 0);
  }
}

void stopBtnPressed() {
  Trigger trigger = Trigger { timeMs_t(millis()), 0, STATION_TRIGGER_TYPE_FINISH };
  if(isDisplaySelect->getValue()) { // master
    masterTrigger(trigger);
  } else {
    slaveTrigger(millis(), STATION_TRIGGER_TYPE_FINISH, 0);
  }
}

void lapBtnPressed() {
  Trigger trigger = Trigger { timeMs_t(millis()), stopWatchLap, STATION_TRIGGER_TYPE_CHECKPOINT };
  if(isDisplaySelect->getValue()) { // master
    masterTrigger(trigger);
  } else {
    slaveTrigger(millis(), STATION_TRIGGER_TYPE_CHECKPOINT, stopWatchLap);
  }
  stopWatchLap++;
}

void startFinishBtnPressed() {
  Trigger trigger = Trigger { timeMs_t(millis()), 0, STATION_TRIGGER_TYPE_START_FINISH };
  if(isDisplaySelect->getValue()) { // master
    masterTrigger(trigger);
  } else {
    slaveTrigger(millis(), STATION_TRIGGER_TYPE_START_FINISH, 0);
  }
}

void timToStr(timeMs_t timeMs, char* str, bool oneMsDigit = false) {
  timeMs = abs(timeMs);
  char hStr[10] = "\0";
  char mStr[3]  = "\0";
  char sStr[3];
  char msStr[4];
  LedMatrix::timeToStr(timeMs, hStr, mStr, sStr, msStr, oneMsDigit);
  if(hStr[0]) {
    sprintf(str, "%s:%s:%s.%s", hStr, mStr, sStr, msStr);
  } else if(mStr[0]) {
    sprintf(str, "%s:%s.%s", mStr, sStr, msStr);
  } else {
    sprintf(str, "%s.%ss", sStr, msStr);
  }
}

#define VIEWER_MAX_TRIGGERS 15

// void showOlder() {

// }

// void showNewer() {

// }

// size_t viewerLapOffset = 0;

/**
 * Viewer variables
 */
char* viewerTimeStrings[1000];
size_t viewerTimeStringsCount = 0;
MenuItem* viewerMenuItems[1000];
size_t viewerMenuItemCount = 0;

// Button* showOlderBtn = new Button("Show older", showOlder);
// Button* showNewerBtn = new Button("Show older", showNewer);

// void addCheckpointToViewer(timeMs_t checkpointDurationMs, uint16_t millimeters, uint16_t currentCheckpoint, uint8_t triggerType) {
//     char* timeStr = new char[20];
//     if(triggerType == STATION_TRIGGER_TYPE_CHECKPOINT) {
//       sprintf(timeStr, "# %i %im | ", currentCheckpoint + 1, int(millimeters / 1000));
//     } else {
//       sprintf(timeStr, "# %i | ", currentCheckpoint + 1);
//     }
//     viewerTimeStrings[viewerTimeStringsCount++] = timeStr;
//     timToStr(checkpointDurationMs, timeStr + strlen(timeStr));
//     TextItem* textItem = new TextItem(timeStr, true, TEXT_ALIGN_LEFT);
//     viewerMenuItems[viewerMenuItemCount++] = textItem;
//     viewerMenu->prependItem(textItem, false);
// }

// void addLapToViewer(timeMs_t lapDurationMs, int16_t lapCount) {
//     char* timeStr = new char[25];
//     sprintf(timeStr, "Lap %i | ", lapCount);
//     viewerTimeStrings[viewerTimeStringsCount++] = timeStr;
//     timToStr(lapDurationMs, timeStr + strlen(timeStr));
//     TextItem* textItem = new TextItem(timeStr, true, TEXT_ALIGN_LEFT);
//     viewerMenuItems[viewerMenuItemCount++] = textItem;
//     viewerMenu->prependItem(textItem, false);
// }

// void addLineToViewer() {
//   Seperator* seperator = new Seperator();
//   viewerMenuItems[viewerMenuItemCount++] = seperator;
//   viewerMenu->prependItem(seperator, false);
// }

// void updateViewer() {
//   TrainingsSession& session = spiffsLogic.getActiveTraining();
//   viewerMenu->removeAll();
  
//   for (size_t i = 0; i < viewerMenuItemCount; i++) {
//     delete viewerMenuItems[i];
//   }
//   viewerMenuItemCount = 0;

//   for (size_t i = 0; i < viewerTimeStringsCount; i++) {
//     delete[] viewerTimeStrings[i];
//   }
//   viewerTimeStringsCount = 0;

//   timeMs_t lapStart = 0;
//   timeMs_t lastTrigger = 0;
//   int16_t lastMillimeters = -1;
//   int16_t lapCount = 1 + max(0, int(session.getTriggerCount()) - VIEWER_MAX_TRIGGERS); // start at 1 to have the first display as 1
//   bool checkpointPassed = false;
//   bool lapStarted = false;
//   uint16_t currentCheckpoint = 0;
//   for (size_t i = max(0, int(session.getTriggerCount()) - VIEWER_MAX_TRIGGERS); i < session.getTriggerCount(); i++) {
//     const Trigger& t = session.getTrigger(i);
//     if(lapStarted && (t.triggerType == STATION_TRIGGER_TYPE_FINISH || t.triggerType == STATION_TRIGGER_TYPE_START_FINISH)) {
//       char* timeStr = new char[15];
//       viewerTimeStrings[viewerTimeStringsCount++] = timeStr;
//       if(checkpointPassed) {
//         addCheckpointToViewer(t.timeMs - lastTrigger, t.millimeters, currentCheckpoint, t.triggerType);
//       }
//       addLapToViewer(t.timeMs - lapStart, lapCount);
//       addLineToViewer();
//       lapStarted = false;
//       lapCount++;
//     }
//     if(t.triggerType == STATION_TRIGGER_TYPE_START || t.triggerType == STATION_TRIGGER_TYPE_START_FINISH) {
//       lapStart = t.timeMs;
//       lastTrigger = t.timeMs;
//       lastMillimeters = -1;
//       checkpointPassed = false;
//       lapStarted = true;
//       currentCheckpoint = 0;
//     }
//     if(lapStarted && t.triggerType == STATION_TRIGGER_TYPE_CHECKPOINT) {
//       if(int(t.millimeters) <= lastMillimeters) continue;
//       addCheckpointToViewer(t.timeMs - lastTrigger, t.millimeters, currentCheckpoint, t.triggerType);
//       lastMillimeters = t.millimeters;
//       lastTrigger = t.timeMs;
//       checkpointPassed = true;
//       currentCheckpoint++;
//     }
//   }
//   if(lapStarted && !checkpointPassed) {
//     TextItem* runningTxt = new TextItem("Running...");
//     viewerMenuItems[viewerMenuItemCount++] = runningTxt;
//     viewerMenu->prependItem(runningTxt, false);
//   }

//   // char* fileNameText = new char[20];
//   // viewerTimeStrings[viewerTimeStringsCount++] = fileNameText;
//   // sprintf(fileNameText, "File: %s", session->getFileName());
//   // TextItem* fileNameLabel = new TextItem(fileNameText, true);
//   // viewerMenuItems[viewerMenuItemCount++] = fileNameLabel;
//   // viewerMenu->prependItem(fileNameLabel);
  
//   // TextItem* viewerLabel = new TextItem("Viewer");
//   // viewerMenuItems[viewerMenuItemCount++] = viewerLabel;
//   // viewerMenu->prependItem(viewerLabel);
// }

void trainingsModeChanged() {
  Serial.println("trainingsMode changed");
  targetTimeSubMenu->setHidden(trainingsModeSelect->getValue() != TRAININGS_MODE_TARGET);
}

void stationTypeChanged() {
  if(stationTypeSelect->getValue() == STATION_TRIGGER_TYPE_START) {
    distFromStartInput->setHidden(true);
  } else {
    distFromStartInput->setHidden(false);
  }
}

// void wifiEnabledChanged() {
//   setWiFiActive(wifiEnabledCB->isChecked());
//   wifiSSIDText->setHidden(!wifiEnabledCB->isChecked());
//   wifiPasswdText->setHidden(!wifiEnabledCB->isChecked());
//   wifiIPText->setHidden(!wifiEnabledCB->isChecked());
//   wifiHelpText1->setHidden(!wifiEnabledCB->isChecked());
//   wifiHelpText2->setHidden(!wifiEnabledCB->isChecked());
//   wifiHelpText3->setHidden(!wifiEnabledCB->isChecked());
// }

void deleteAllSessionsPressed() {
  spiffsLogic.deleteAllSessions();
}

void beginLCDDisplay() {
  distFromStartInput = new NumberField("Dist. from start", "m", 0.1, 0, 655, 1, 10, simpleInputChanged);
  minDelayInput = new NumberField("Min. delay", "s", 0.1, 0.5, 1000, 1, 0, simpleInputChanged);

  displayCurrentText = new NumberField("Disp.", "A", 0.01, 0, 100, 2);
  displayCurrentText->setEditable(false);
  displayCurrentAfterScaleText = new NumberField("Disp. scaled", "A", 0.01, 0, 100, 2);
  displayCurrentAfterScaleText->setEditable(false);
  vBatMeasured = new NumberField("vMeasured", "V", 0.01, 0, 100, 2);
  vBatMeasured->setEditable(false);
  vBatText = new NumberField("vBat", "V", 0.01, 0, 100, 2);
  vBatText->setEditable(false);
  hzText = new NumberField("Loop", "Hz", 1, 0, 100000000, 0);
  hzText->setEditable(false);
  displayBrightnessInput = new NumberField("Brightness", "%", 1, 5, 100, 0, 30, simpleInputChanged);
  displayTimeInput = new NumberField("Display time", "s", 0.5, 0.5, 100, 1, 3, simpleInputChanged);
  freeHeapText = new NumberField("Free", "b", 1, 0, UINT16_MAX, 0);
  freeHeapText->setEditable(false);
  heapSizeText = new NumberField("Size", "b", 1, 0, UINT16_MAX, 0);
  heapSizeText->setEditable(false);
  showAdvancedCB = new CheckBox("Show advanced", false, false, showDebugChanged);
  advancedText = new TextItem("Advanced");
  trainingsModeSelect = new Select("Trainings mode", trainingsModeChanged);
  trainingsModeSelect->addOption("Normal", "Normal");
  trainingsModeSelect->addOption("Target time", "Target");
  stationTypeSelect = new Select("Function", stationTypeChanged);
  stationTypeSelect->addOption("Start + finish", "S+F"); // STATION_TRIGGER_TYPE_START_FINISH 0
  stationTypeSelect->addOption("Only start", "Start"); // STATION_TRIGGER_TYPE_START 1
  stationTypeSelect->addOption("checkpoint", "Checkpoint"); // STATION_TRIGGER_TYPE_CHECKPOINT 2
  stationTypeSelect->addOption("Only finish", "Finish"); // STATION_TRIGGER_TYPE_FINISH 3
  stationTypeChanged();

  lapDisplayTypeSelect = new Select("Lap display");
  lapDisplayTypeSelect->addOption("Lap time", "Time");
  lapDisplayTypeSelect->addOption("Avg. lap speed", "Speed");
  lapDisplayTypeSelect->addOption("Parcour mode", "Parcour");

  laserValue = new NumberField("Analog laser", "", 1, 0, UINT16_MAX, 0);
  isDisplaySelect = new Select("Station type", isDisplayChanged);
  isDisplaySelect->addOption("Laser/Reflector", "Laser");
  isDisplaySelect->addOption("Display station", "Display");
  char* wifiPasswdTextStr = new char[20]; // never deleted
  sprintf(wifiPasswdTextStr, "Password: %s", APPassword);
  wifiPasswdText = new TextItem(wifiPasswdTextStr, true, DISPLAY_TEXT_ALIGNMENT::TEXT_ALIGN_LEFT);
  char* wifiSsidTextStr = new char[20]; // never deleted
  sprintf(wifiSsidTextStr, "Name: %s", APSsid);
  wifiSSIDText = new TextItem(wifiSsidTextStr, true, DISPLAY_TEXT_ALIGNMENT::TEXT_ALIGN_LEFT);
  wifiIPText = new TextItem("URL: http://8.8.8.8", true, DISPLAY_TEXT_ALIGNMENT::TEXT_ALIGN_LEFT);
  wifiHelpText1 = new TextItem("Troubleshooting:", true, DISPLAY_TEXT_ALIGNMENT::TEXT_ALIGN_LEFT);
  wifiHelpText2 = new TextItem("Double check that https", true, DISPLAY_TEXT_ALIGNMENT::TEXT_ALIGN_LEFT);
  wifiHelpText3 = new TextItem("is not used. Use http!", true, DISPLAY_TEXT_ALIGNMENT::TEXT_ALIGN_LEFT);

  inPositionDelay = new NumberField("Delay", "s", 1, 0, UINT16_MAX, 0, 0);

  setMinDelay = new NumberField("Min delay", "s", 1, 0, UINT16_MAX, 0, 0, setMinDelayChanged);
  setMaxDelay = new NumberField("Max delay", "s", 1, 0, UINT16_MAX, 0, 0, setMaxDelayChanged);

  goMinDelay = new NumberField("Min delay", "s", 1, 0, UINT16_MAX, 0, 0, goMinDelayChanged);
  goMaxDelay = new NumberField("Max delay", "s", 1, 0, UINT16_MAX, 0, 0, goMaxDelayChanged);
  resetStartGun();

  targetTimeInput = new TimeInput("Target time", 0, 1000000000, 1000, 0);
  targetMetersInput = new NumberField("Distance", "m", 1, 100, 100000000, 0, 0);
  targetMetersPerLapInput = new NumberField("Meters/Lap", "m", 1, 10, 10000000, 0, 0);
  resetTarget();

  // Button* deleteAllSessionsBtn = new Button("Delete all sessions", deleteAllSessionsPressed);
  Button* startGunBtn = new Button("Start now!", startGunNowBtnPressed);
  Button* startGun15sBtn = new Button("Start in 15s", startGun15sBtnPressed);

  startBtn = new Button("Start", startBtnPressed);
  lapBtn = new Button("Lap", lapBtnPressed);
  stopBtn = new Button("Stop", stopBtnPressed);
  startFinishBtn = new Button("Start/Finish", startFinishBtnPressed);
  
  // wifiEnabledCB = new CheckBox("WiFi Activated", false, false, wifiEnabledChanged);

  cloudUploadEnabled = new CheckBox("Cloud upload", false, false, cloudUploadChanged);

  Button* uploadNowBtn = new Button("Upload now", tryInitUpload);

  fontSizeSelect = new Select("Display font size", writePreferences);
  fontSizeSelect->addOption("Large", "L");
  fontSizeSelect->addOption("Small", "S");

  // All menu inits
  Menu* systemSettingsMenu = new Menu();
  Menu* setupMenu = new Menu();
  Menu* debugMenu = new Menu();
  Menu* menuFactoryReset = new Menu("No");
  // connectionsMenuMaster = new Menu();
  // viewerMenu = new Menu();
  Menu* wifiMenu = new Menu();
  Menu* infoMenu = new Menu();
  Menu* startMenu = new Menu();
  targetTimeMenu = new Menu();
  targetTimeSubMenu = new SubMenu("Target time", targetTimeMenu);
  debugSubMenu = new SubMenu("Debug", debugMenu);
  debugSubMenu->setHidden(true);

  setupMenu->addItem(new TextItem("Setup"));
  // setupMenu->addItem(trainingsModeSelect); // future version
  setupMenu->addItem(targetTimeSubMenu);

    targetTimeMenu->addItem(new TextItem("Target time"));
    targetTimeMenu->addItem(targetTimeInput);
    targetTimeMenu->addItem(targetMetersInput);
    targetTimeMenu->addItem(targetMetersPerLapInput);
    targetTimeMenu->addItem(new Button("Reset target", resetTarget));

  setupMenu->addItem(stationTypeSelect);
  setupMenu->addItem(distFromStartInput);
  setupMenu->addItem(displayTimeInput);
  setupMenu->addItem(minDelayInput);
  setupMenu->addItem(displayBrightnessInput);
  setupMenu->addItem(lapDisplayTypeSelect);
  setupMenu->addItem(fontSizeSelect);

    startMenu->addItem(new TextItem("Start gun"));
    startMenu->addItem(startGunBtn);
    startMenu->addItem(startGun15sBtn);
    startMenu->addItem(new TextItem("\"Set\""));
    startMenu->addItem(setMinDelay);
    startMenu->addItem(setMaxDelay);
    startMenu->addItem(new TextItem("\"Go!\""));
    startMenu->addItem(goMinDelay);
    startMenu->addItem(goMaxDelay);
    startMenu->addItem(new Button("Reset start gun", resetStartGun));
    startMenu->addItem(new TextItem("Stop watch"));
    startMenu->addItem(startBtn);
    startMenu->addItem(lapBtn);
    startMenu->addItem(stopBtn);
    startMenu->addItem(startFinishBtn);

  setupMenu->addItem(new SubMenu("System settings", systemSettingsMenu));

    systemSettingsMenu->addItem(new TextItem("System settings"));

    systemSettingsMenu->addItem(new SubMenu("Factory reset", menuFactoryReset));

      menuFactoryReset->addItem(new TextItem("Are you sure?"));
      menuFactoryReset->addItem(new Button("Yes", factoryReset));

    systemSettingsMenu->addItem(showAdvancedCB);
    systemSettingsMenu->addItem(advancedText);
    systemSettingsMenu->addItem(debugSubMenu);
    systemSettingsMenu->addItem(isDisplaySelect);
    // systemSettingsMenu->addItem(deleteAllSessionsBtn);

      debugMenu->addItem(new TextItem("Info for nerds"));
      debugMenu->addItem(displayCurrentText);
      debugMenu->addItem(displayCurrentAfterScaleText);
      debugMenu->addItem(vBatMeasured);
      debugMenu->addItem(vBatText);
      debugMenu->addItem(hzText);
      debugMenu->addItem(new TextItem("Heap memory"));
      debugMenu->addItem(freeHeapText);
      debugMenu->addItem(heapSizeText);
      debugMenu->addItem(laserValue);
      debugMenu->addItem(new Button("Reboot", reboot));

  setupMenu->addItem(new SubMenu("Info", infoMenu));

    infoMenu->addItem(new TextItem("Roller timing", true));
    infoMenu->addItem(new TextItem("Version", true));
    infoMenu->addItem(new TextItem(VERSION, true));
    infoMenu->addItem(new TextItem("www.roller-results.com", true));
    infoMenu->addItem(new TextItem("By Timo Lehenrtz", true));

  // viewerMenu->addItem(new TextItem("Viewer"));


  // connectionsMenuMaster->addItem(new TextItem("Connections"));

  wifiMenu->addItem(new TextItem("WiFi", false));
  // wifiMenu->addItem(wifiEnabledCB);
  wifiMenu->addItem(wifiSSIDText);
  wifiMenu->addItem(wifiPasswdText);
  wifiMenu->addItem(wifiIPText);
  wifiMenu->addItem(wifiHelpText1);
  wifiMenu->addItem(wifiHelpText2);
  wifiMenu->addItem(wifiHelpText3);
  wifiMenu->addItem(cloudUploadEnabled);
  wifiMenu->addItem(uploadNowBtn);

  frameSections[0] = FrameSection(drawSetup, setupMenu);
  frameSections[1] = FrameSection(drawStartGun, startMenu);
  // frameSections[2] = FrameSection(drawConnections, connectionsMenuMaster);
  // frameSections[2] = FrameSection(drawViewer, viewerMenu);
  frameSections[2] = FrameSection(drawFrameWiFi, wifiMenu);

  uiManager.begin(overlayCallbacks, overlaysCount, frameSections, 3);

  // for (size_t i = 0; i < MAX_CONNECTIONS; i++) {
  //   connectionItems[i] = nullptr;
  // }
  // for (size_t i = 0; i < MAX_CONNECTIONS; i++) {
  //   connectionItemsTexts[i] = nullptr;
  // }
  showDebugChanged();
  // wifiEnabledChanged();
}

void beginLEDDisplay() {
  FastLED.clearData();
  FastLED.addLeds<NEOPIXEL, PIN_WS2812b>(leds, NUM_LEDS_DISPLAY);
  matrix = LedMatrix(leds, NUM_LEDS_DISPLAY, pixelConverter);
}

/**
 * @brief probably acurate +-10%
 */
float predictLEDCurrentDraw() {
  float currentDraw = 0; // w
  for (size_t i = 0; i < getLEDCount(); i++) {
    currentDraw += leds[i].r / 255.0 * MAX_AMPS_PER_PIXEL / 3.0;
    currentDraw += leds[i].g / 255.0 * MAX_AMPS_PER_PIXEL / 3.0;
    currentDraw += leds[i].b / 255.0 * MAX_AMPS_PER_PIXEL / 3.0;
  }
  return currentDraw;
}

timeMs_t lastLEDUpdate = 0;

void ledDisplayTime(timeMs_t time, bool oneDigit) {
  if(fontSizeSelect->getValue() == 0) {// large
    matrix.printTimeBig(6, 0, time, oneDigit);
  } else { // small
    matrix.printTimeSmall(10, 2, time, oneDigit);
  }
}

void ledDisplaySpeed(float speed) {
  if(fontSizeSelect->getValue() == 0) {// large
    matrix.printSpeedBig(6, 0, speed);
  } else { // small
    matrix.printSpeedSmall(10, 2, speed);
  }
}

bool isLaserTimeout() {
  return millis() < lastTriggerMs + minDelayInput->getValue() * 1000 && lastTriggerMs != 0;
}

void handleLEDS() {
  float stationDisplayBrightness = 1;
  static float lastDisplayBrightness = 0.3;
  if(millis() - lastLEDUpdate < 1000.0 / 30.0) return;
  lastLEDUpdate = millis();
  FastLED.clear();
  TrainingsSession& session = spiffsLogic.getActiveTraining();
  if(isDisplaySelect->getValue()) {
    if(millis() < 4000) { // intro
      matrix.print("GO SKATE!", 1, 1, CRGB::White, FONT_SIZE_SMALL + FONT_SETTINGS_DEFAULT);
    } else { // display lap
      if(lapDisplayTypeSelect->getValue() == 0) { // lap time
        if(session.getTriggerCount() == 0) {
          ledDisplayTime(0, true);
        } else {
          if((session.isLapStarted() || session.getTimeSinceLastFinish() < displayTimeInput->getValue() * 1000) && session.getTimeSinceLastSplit() < displayTimeInput->getValue() * 1000 / 2) {
            ledDisplayTime(session.getLastSplitTime(), false);
          } else if(session.getLastLapMs() != INT32_MAX && (session.getTimeSinceLastFinish() < displayTimeInput->getValue() * 1000 || !session.isLapStarted())) {
            ledDisplayTime(session.getLastLapMs(), false);
          } else {
            timeMs_t time = session.getTimeSinceLastStart();
            if(time == INT32_MAX) time = 0;
            // time = time / 100 * 100; // getting last 2 digits to 0
            ledDisplayTime(time, true);
          }
        }
      } else { // lap speed
        if(session.getTriggerCount() == 0) {
          ledDisplayTime(0, true);
        } else {
          if((session.isLapStarted() || session.getTimeSinceLastFinish() < displayTimeInput->getValue() * 1000) && session.getTimeSinceLastSplit() < displayTimeInput->getValue() * 1000 / 2) {
            ledDisplayTime(session.getLastSplitTime(), false);
          } else if(session.getLastLapMs() != INT32_MAX && (session.getTimeSinceLastFinish() < displayTimeInput->getValue() * 1000 || !session.isLapStarted())) {
            const timeMs_t lastLapTime = session.getLastLapMs();
            if(lastLapTime != 0) {
              const float speed = (session.getLastLapDistance() / 1000000.0) / (lastLapTime / 1000.0 / 60 / 60); // km / h
              Serial.printf("dist: %fkm, time: %fh, speed: %fkph\n", session.getLastLapDistance() / 1000000.0, (lastLapTime / 1000.0 / 60 / 60), speed);
              ledDisplaySpeed(speed);
            }
          } else {
            ledDisplayTime(0, true);
          }
        }
      }
      // Lap count
      char lapsStr[3];
      size_t laps = session.getLapsCount();
      if(fontSizeSelect->getValue() == 0) { // large
        laps = laps % 10;
      } else {
        laps = laps % 100;
      }
      sprintf(lapsStr, "%i", laps);
      matrix.print(lapsStr, 0, 0, CRGB::Yellow, FONT_SIZE_SMALL + FONT_SETTINGS_DEFAULT);
    }
  } else {
    static size_t ledState = 0;
    size_t prevLedState = ledState;
    static timeMs_t lastLEDStateChange = 0;
    for (int i = 0; i < min(double(getLEDCount()), millis() / 150.0 - 7); i++) {
      if(isTriggered()) {
        ledState = 1;
        leds[i] = CRGB::White;
      } else if(isLaserTimeout()) {
        ledState = 2;
        leds[i] = CRGB::Red;
      } else {
        ledState = 3;
        double value = (sin(((millis() + timeSyncOffset) - i * 750) / 1000.0) + 1.0) / 2.0;
        leds[i] = CRGB(60 * value, 0, 40 * value);
      }
    }
    if(prevLedState != ledState) {
      lastLEDStateChange = millis();
    }
    if(millis() - lastLEDStateChange < 3000) {
      stationDisplayBrightness = 1;
    } else {
      stationDisplayBrightness = 0.001;
    }
  }

  // debug
  freeHeapText->setValue(ESP.getFreeHeap());
  heapSizeText->setValue(ESP.getHeapSize());
  laserValue->setValue(digitalRead(PIN_LASER));

  // handle brightness
  float displayCurrent = predictLEDCurrentDraw();
  displayCurrentText->setValue(displayCurrent);
  const float inputBrightness = isDisplaySelect->getValue() ? displayBrightnessInput->getValue() / 100.0 * 255.0 : stationDisplayBrightness * 255.0;
  float displayBrightness = MAX_CONTINUOUS_AMPS / displayCurrent * inputBrightness;
  if(displayBrightness > 150) displayBrightness = 150;
  displayCurrentAfterScaleText->setValue(displayCurrent * (displayBrightness / 255.0));
  float brightnessLPF = 0.05;
  displayBrightness = displayBrightness * brightnessLPF + lastDisplayBrightness * (1 - brightnessLPF);
  lastDisplayBrightness = displayBrightness;
  FastLED.setBrightness(displayBrightness);
  FastLED.show();
  if(millis() < 1000) {
    digitalWrite(PIN_LED_WHITE, millis() % 100 > 50);
  } else {
    digitalWrite(PIN_LED_WHITE, LOW);
  }
}