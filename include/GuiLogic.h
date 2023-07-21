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

#define MASTER_FRAMES 5
#define SLAVE_FRAMES 2

TextItem* connectionItems[MAX_CONNECTIONS];
char* connectionItemsTexts[MAX_CONNECTIONS];

size_t connectionCount = 0;

void beginLEDDisplay();
void trainingsModeChanged();

int getLEDCount() {
  return isDisplaySelect->getValue() ? NUM_LEDS_DISPLAY : NUM_LEDS_LASER;
}

uint16_t pixelConverter(uint16_t x, uint16_t y) {
  y = 7 - y;
  return (8 * 31) - x * 8 + (x % 2 == 0 ? (7 - y) : y);
}

void powerSavingChanged() {
  switch(powerSavingSelect->getValue()) {
    case POWER_SAVING_MODE_OFF: {
      masterSlave.setComunicationDelay(0);
      // radio.setOutputPower(22); // full power (150mw)
      break;
    }
    case POWER_SAVING_MODE_MEDIUM: {
      masterSlave.setComunicationDelay(200);
      // radio.setOutputPower(19); // 80mw
      break;
    }
    case POWER_SAVING_MODE_HIGH: {
      masterSlave.setComunicationDelay(400);
      // radio.setOutputPower(9); // 8 mw
      break;
    }
  }
}

void showDebugChanged() {
  debugSubMenu->setHidden(!showAdvancedCB->isChecked());
  isMasterCB->setHidden(!showAdvancedCB->isChecked());
  advancedText->setHidden(!showAdvancedCB->isChecked());
  isDisplaySelect->setHidden(!showAdvancedCB->isChecked());
}

void simpleInputChanged() {
  writePreferences();
}

void isMasterChanged() {
  masterSlave.setMaster(isMasterCB->isChecked());
  if(isMasterCB->isChecked()) {
    spiffsLogic.startNewSession();
    uiManager.begin(overlayCallbacks, overlaysCount, frameSections, MASTER_FRAMES);
    trainingsModeChanged();
  } else {
    spiffsLogic.endSession();
    uiManager.begin(overlayCallbacks, overlaysCount, frameSections, SLAVE_FRAMES);
    targetTimeSubMenu->setHidden(true);
  }
  uidInput->setHidden(isMasterCB->isChecked());
  trainingsModeSelect->setHidden(!isMasterCB->isChecked());
  writePreferences();
}

void reboot() {
  ESP.restart();
}

void isDisplayChanged() {
  isMasterCB->setChecked(isDisplaySelect->getValue());
  isMasterChanged();
  // only on displays
  displayBrightnessInput->setHidden(!isDisplaySelect->getValue());
  displayTimeInput->setHidden(!isDisplaySelect->getValue());
  powerSavingSelect->setHidden(!isDisplaySelect->getValue());
  // only on lasers
  stationTypeSelect->setHidden(isDisplaySelect->getValue());
  minDelayInput->setHidden(isDisplaySelect->getValue());
  writePreferences();
}

void guiSetConnection(uint8_t address, int millimeters, uint8_t lq, uint8_t stationType) {
  if(stationType == STATION_TRIGGER_TYPE_NONE) return;
  if(address >= MAX_CONNECTIONS) return;
  if(connectionItemsTexts[address] == nullptr) {
    connectionItemsTexts[address] = new char[40];
    connectionCount++;
    playSoundNewConnection();
  }
  switch(stationType) {
    case STATION_TRIGGER_TYPE_CHECKPOINT: {
      sprintf(connectionItemsTexts[address], "Checkpoint %.1fm(Lq%i%)", millimeters / 1000.0, lq);
      break;
    }
    case STATION_TRIGGER_TYPE_FINISH: {
      sprintf(connectionItemsTexts[address], "Finish (Lq%i%)", lq);
      break;
    }
    case STATION_TRIGGER_TYPE_START: {
      sprintf(connectionItemsTexts[address], "Start (Lq%i%)", lq);
      break;
    }
    case STATION_TRIGGER_TYPE_START_FINISH: {
      sprintf(connectionItemsTexts[address], "Start + finish (Lq%i%)", lq);
      break;
    }
    default: {
      connectionItemsTexts[address][1] = '-';
      connectionItemsTexts[address][0] = 0;
      break;
    }
  }
  if(connectionItems[address] == nullptr) {
    connectionItems[address] = new TextItem(connectionItemsTexts[address], true);
    connectionsMenuMaster->addItem(connectionItems[address]);
  }
}

void guiRemoveConnection(uint8_t address) {
  if(address >= MAX_CONNECTIONS) return;
  if(connectionItems[address] != nullptr) {
    connectionsMenuMaster->removeItem(connectionItems[address]);
    delete connectionItems[address];
    connectionItems[address] = nullptr;
    connectionCount--;
    playSoundLostConnection();
  }
  if(connectionItemsTexts[address] != nullptr) {
    delete[] connectionItemsTexts[address];
    connectionItemsTexts[address] = nullptr;
  }
}

void msOverlay(ScreenDisplay *display, DisplayUiState* state) {
  display->setColor(BLACK);
  display->fillRect(0, 0, 128, 13);
  display->setColor(WHITE);
  display->fillRect(7, 13, 114, 1);

  char strConnection[20];
  if(masterSlave.isMaster()) {
    if(connectionCount == 0) {
      sprintf(strConnection, "No connections");
    } else {
      sprintf(strConnection, "%i connections", connectionCount);
    }
  } else {
    if(masterSlave.isMasterConnected()) {
      sprintf(strConnection, "Connected");
    } else {
      sprintf(strConnection, "Not connected");
    }
  }

  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 0, String(strConnection));
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

void drawConnections(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display->setFont(ArialMT_Plain_16);
  display->drawStringMaxWidth(x + 64, y + 32, 100, "Connections");
}

void inPositionMinDelayChanged() {
  inPositionMaxDelay->setValue(max(inPositionMaxDelay->getValue(), inPositionMinDelay->getValue()));
}

void inPositionMaxDelayChanged() {
  inPositionMinDelay->setValue(min(inPositionMaxDelay->getValue(), inPositionMinDelay->getValue()));
}

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
  inPositionMinDelay->setValue(15);
  inPositionMaxDelay->setValue(15);
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

void startGunBtnPressed() {
  triggerStartGun();
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

void addCheckpointToViewer(timeMs_t checkpointDurationMs, uint16_t millimeters, uint16_t currentCheckpoint, uint8_t triggerType) {
    char* timeStr = new char[20];
    if(triggerType == STATION_TRIGGER_TYPE_CHECKPOINT) {
      sprintf(timeStr, "# %i %im | ", currentCheckpoint + 1, int(millimeters / 1000));
    } else {
      sprintf(timeStr, "# %i | ", currentCheckpoint + 1);
    }
    viewerTimeStrings[viewerTimeStringsCount++] = timeStr;
    timToStr(checkpointDurationMs, timeStr + strlen(timeStr));
    TextItem* textItem = new TextItem(timeStr, true, TEXT_ALIGN_LEFT);
    viewerMenuItems[viewerMenuItemCount++] = textItem;
    viewerMenu->prependItem(textItem, false);
}

void addLapToViewer(timeMs_t lapDurationMs, int16_t lapCount) {
    char* timeStr = new char[25];
    sprintf(timeStr, "Lap %i | ", lapCount);
    viewerTimeStrings[viewerTimeStringsCount++] = timeStr;
    timToStr(lapDurationMs, timeStr + strlen(timeStr));
    TextItem* textItem = new TextItem(timeStr, true, TEXT_ALIGN_LEFT);
    viewerMenuItems[viewerMenuItemCount++] = textItem;
    viewerMenu->prependItem(textItem, false);
}

void addLineToViewer() {
  Seperator* seperator = new Seperator();
  viewerMenuItems[viewerMenuItemCount++] = seperator;
  viewerMenu->prependItem(seperator, false);
}

void updateViewer() {
  TrainingsSession* session = spiffsLogic.getCurrentSession();
  if(!session) return;
  viewerMenu->removeAll();
  
  for (size_t i = 0; i < viewerMenuItemCount; i++) {
    delete viewerMenuItems[i];
  }
  viewerMenuItemCount = 0;

  for (size_t i = 0; i < viewerTimeStringsCount; i++) {
    delete[] viewerTimeStrings[i];
  }
  viewerTimeStringsCount = 0;
  
  session->sortTriggers(); // probybly not neccessary

  timeMs_t lapStart = 0;
  timeMs_t lastTrigger = 0;
  uint16_t lastMillimeters = 0;
  int16_t lapCount = 1 + max(0, int(session->getTriggerCount()) - VIEWER_MAX_TRIGGERS); // start at 1 to have the first display as 1
  bool checkpointPassed = false;
  bool lapStarted = false;
  uint16_t currentCheckpoint = 0;
  for (size_t i = max(0, int(session->getTriggerCount()) - VIEWER_MAX_TRIGGERS); i < session->getTriggerCount(); i++) {
    Trigger* t = session->getTrigger(i);
    if(lapStarted && (t->triggerType == STATION_TRIGGER_TYPE_FINISH || t->triggerType == STATION_TRIGGER_TYPE_START_FINISH)) {
      char* timeStr = new char[15];
      viewerTimeStrings[viewerTimeStringsCount++] = timeStr;
      if(checkpointPassed) {
        addCheckpointToViewer(t->timeMs - lastTrigger, t->millimeters, currentCheckpoint, t->triggerType);
      }
      addLapToViewer(t->timeMs - lapStart, lapCount);
      addLineToViewer();
      lapStarted = false;
      lapCount++;
    }
    if(t->triggerType == STATION_TRIGGER_TYPE_START || t->triggerType == STATION_TRIGGER_TYPE_START_FINISH) {
      lapStart = t->timeMs;
      lastTrigger = t->timeMs;
      lastMillimeters = 0;
      checkpointPassed = false;
      lapStarted = true;
      currentCheckpoint = 0;
    }
    if(lapStarted && t->triggerType == STATION_TRIGGER_TYPE_CHECKPOINT) {
      if(t->millimeters <= lastMillimeters) continue;
      addCheckpointToViewer(t->timeMs - lastTrigger, t->millimeters, currentCheckpoint, t->triggerType);
      lastMillimeters = t->millimeters;
      lastTrigger = t->timeMs;
      checkpointPassed = true;
      currentCheckpoint++;
    }
  }

  // char* fileNameText = new char[20];
  // viewerTimeStrings[viewerTimeStringsCount++] = fileNameText;
  // sprintf(fileNameText, "File: %s", session->getFileName());
  // TextItem* fileNameLabel = new TextItem(fileNameText, true);
  // viewerMenuItems[viewerMenuItemCount++] = fileNameLabel;
  // viewerMenu->prependItem(fileNameLabel);
  
  // TextItem* viewerLabel = new TextItem("Viewer");
  // viewerMenuItems[viewerMenuItemCount++] = viewerLabel;
  // viewerMenu->prependItem(viewerLabel);
}

void trainingsModeChanged() {
  Serial.println("trainingsMode changed");
  targetTimeSubMenu->setHidden(trainingsModeSelect->getValue() != TRAININGS_MODE_TARGET);
}

void stationTypeChanged() {
  distFromStartInput->setHidden(stationTypeSelect->getValue() != STATION_TRIGGER_TYPE_CHECKPOINT);
}

void wifiEnabledChanged() {
  setWiFiActive(wifiEnabledCB->isChecked());
  wifiSSIDText->setHidden(!wifiEnabledCB->isChecked());
  wifiPasswdText->setHidden(!wifiEnabledCB->isChecked());
  wifiIPText->setHidden(!wifiEnabledCB->isChecked());
}

void beginLCDDisplay() {
  distFromStartInput = new NumberField("Dist. from start", "m", 0.1, 0, 100000, 1, 0, simpleInputChanged);
  minDelayInput = new NumberField("Min. delay", "s", 0.1, 0.5, 1000, 1, 0, simpleInputChanged);
  isMasterCB = new CheckBox("Is master", false, false, isMasterChanged);

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
  uidInput = new NumberField("Uid", "", 1, 0, MAX_CONNECTIONS, 0, 0, simpleInputChanged);
  displayBrightnessInput = new NumberField("Brightness", "%", 1, 5, 100, 0, 30, simpleInputChanged);
  displayTimeInput = new NumberField("Lap dispplay", "s", 0.5, 0.5, 100, 1, 3, simpleInputChanged);
  powerSavingSelect = new Select("Power saving", powerSavingChanged);
  powerSavingSelect->addOption("Off", "Off");
  powerSavingSelect->addOption("Medium", "Medium");
  powerSavingSelect->addOption("Energy saver", "High");
  freeHeapText = new NumberField("Free", "b", 1, 0, UINT16_MAX, 0);
  freeHeapText->setEditable(false);
  heapSizeText = new NumberField("Size", "b", 1, 0, UINT16_MAX, 0);
  heapSizeText->setEditable(false);
  showAdvancedCB = new CheckBox("Show advanced", false, false, showDebugChanged);
  advancedText = new TextItem("Advanced");
  trainingsModeSelect = new Select("Trainings mode", trainingsModeChanged);
  trainingsModeSelect->addOption("Normal", "Normal");
  trainingsModeSelect->addOption("Target time", "Target");
  stationTypeSelect = new Select("Location", stationTypeChanged);
  stationTypeSelect->addOption("Start + finish", "S+F"); // STATION_TRIGGER_TYPE_START_FINISH 0
  stationTypeSelect->addOption("Only start", "start"); // STATION_TRIGGER_TYPE_START 1
  stationTypeSelect->addOption("checkpoint", "checkpoint"); // STATION_TRIGGER_TYPE_CHECKPOINT 2
  stationTypeSelect->addOption("Only finish", "finish"); // STATION_TRIGGER_TYPE_FINISH 3
  stationTypeChanged();
  laserValue = new NumberField("Analog laser", "", 1, 0, UINT16_MAX, 0);
  isDisplaySelect = new Select("Station type", isDisplayChanged);
  isDisplaySelect->addOption("Laser/Reflector", "Laser");
  isDisplaySelect->addOption("Display station", "Display");
  char* wifiPasswdTextStr = new char[20]; // never deleted
  sprintf(wifiPasswdTextStr, "Password: %s", wifiPassword);
  wifiPasswdText = new TextItem(wifiPasswdTextStr, true, DISPLAY_TEXT_ALIGNMENT::TEXT_ALIGN_LEFT);
  char* wifiSsidTextStr = new char[20]; // never deleted
  sprintf(wifiSsidTextStr, "Name: %s", wifiSsid);
  wifiSSIDText = new TextItem(wifiSsidTextStr, true, DISPLAY_TEXT_ALIGNMENT::TEXT_ALIGN_LEFT);
  wifiIPText = new TextItem(wifiIPStr, true, DISPLAY_TEXT_ALIGNMENT::TEXT_ALIGN_LEFT);

  inPositionMinDelay = new NumberField("Min delay", "s", 1, 0, UINT16_MAX, 0, 0, inPositionMinDelayChanged);
  inPositionMaxDelay = new NumberField("Max delay", "s", 1, 0, UINT16_MAX, 0, 0, inPositionMaxDelayChanged);

  setMinDelay = new NumberField("Min delay", "s", 1, 0, UINT16_MAX, 0, 0, setMinDelayChanged);
  setMaxDelay = new NumberField("Max delay", "s", 1, 0, UINT16_MAX, 0, 0, setMaxDelayChanged);

  goMinDelay = new NumberField("Min delay", "s", 1, 0, UINT16_MAX, 0, 0, goMinDelayChanged);
  goMaxDelay = new NumberField("Max delay", "s", 1, 0, UINT16_MAX, 0, 0, goMaxDelayChanged);
  resetStartGun();

  targetTimeInput = new TimeInput("Target time", 0, 1000000000, 1000, 0);
  targetMetersInput = new NumberField("Distance", "m", 1, 100, 100000000, 0, 0);
  targetMetersPerLapInput = new NumberField("Meters/Lap", "m", 1, 10, 10000000, 0, 0);
  resetTarget();

  Button* startGunBtn = new Button("Start!", startGunBtnPressed);
  
  wifiEnabledCB = new CheckBox("WiFi Activated", false, false, wifiEnabledChanged);

  // All menu inits
  Menu* systemSettingsMenu = new Menu();
  Menu* setupMenu = new Menu();
  Menu* debugMenu = new Menu();
  Menu* menuFactoryReset = new Menu("No");
  connectionsMenuMaster = new Menu();
  viewerMenu = new Menu();
  Menu* wifiMenu = new Menu();
  Menu* infoMenu = new Menu();
  Menu* startMenu = new Menu();
  targetTimeMenu = new Menu();
  targetTimeSubMenu = new SubMenu("Target time", targetTimeMenu);
  debugSubMenu = new SubMenu("Debug", debugMenu);
  debugSubMenu->setHidden(true);

  setupMenu->addItem(new TextItem("Setup"));
  setupMenu->addItem(trainingsModeSelect);
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

    startMenu->addItem(new TextItem("\"Start gun\""));
    startMenu->addItem(startGunBtn);
    startMenu->addItem(new TextItem("\"In position\""));
    startMenu->addItem(inPositionMinDelay);
    startMenu->addItem(inPositionMaxDelay);
    startMenu->addItem(new TextItem("\"Set\""));
    startMenu->addItem(setMinDelay);
    startMenu->addItem(setMaxDelay);
    startMenu->addItem(new TextItem("\"Go!\""));
    startMenu->addItem(goMinDelay);
    startMenu->addItem(goMaxDelay);
    startMenu->addItem(new Button("Reset start gun", resetStartGun));

  setupMenu->addItem(new SubMenu("System settings", systemSettingsMenu));

    systemSettingsMenu->addItem(new TextItem("System settings"));
    systemSettingsMenu->addItem(powerSavingSelect);

    systemSettingsMenu->addItem(new SubMenu("Factory reset", menuFactoryReset));

      menuFactoryReset->addItem(new TextItem("Are you sure?"));
      menuFactoryReset->addItem(new Button("Yes", factoryReset));

    systemSettingsMenu->addItem(uidInput);
    systemSettingsMenu->addItem(showAdvancedCB);
    systemSettingsMenu->addItem(advancedText);
    systemSettingsMenu->addItem(isMasterCB);
    systemSettingsMenu->addItem(debugSubMenu);
    systemSettingsMenu->addItem(isDisplaySelect);

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
    infoMenu->addItem(new TextItem(VERSION, true));
    infoMenu->addItem(new TextItem("www.roller-results.com", true));

  viewerMenu->addItem(new TextItem("Viewer"));


  connectionsMenuMaster->addItem(new TextItem("Connections"));

  wifiMenu->addItem(new TextItem("WiFi", false));
  wifiMenu->addItem(wifiEnabledCB);
  wifiMenu->addItem(wifiSSIDText);
  wifiMenu->addItem(wifiPasswdText);
  wifiMenu->addItem(wifiIPText);

  frameSections[0] = FrameSection(drawSetup, setupMenu);
  frameSections[1] = FrameSection(drawStartGun, startMenu);
  frameSections[2] = FrameSection(drawConnections, connectionsMenuMaster);
  frameSections[3] = FrameSection(drawViewer, viewerMenu);
  frameSections[4] = FrameSection(drawFrameWiFi, wifiMenu);

  uiManager.begin(overlayCallbacks, overlaysCount, frameSections, 4);

  for (size_t i = 0; i < MAX_CONNECTIONS; i++) {
    connectionItems[i] = nullptr;
  }
  for (size_t i = 0; i < MAX_CONNECTIONS; i++) {
    connectionItemsTexts[i] = nullptr;
  }
  showDebugChanged();
  wifiEnabledChanged();
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

void handleLEDS() {
  if(millis() - lastLEDUpdate < 1000.0 / 30.0) return;
  lastLEDUpdate = millis();
  FastLED.clear();
  if(isDisplaySelect->getValue()) {
    TrainingsSession* session = spiffsLogic.getCurrentSession();
    size_t laps = 0;
    if(session && session->getTriggerCount() > 0) {
      laps = session->getLapsCount();
      if(session->getTriggerCount() == 1 || session->getTimeSinceLastTrigger() > displayTimeInput->getValue() * 1000) {
        timeMs_t time = session->getTimeSinceLastTrigger();
        time = time / 100 * 100; // getting last 2 digits to 0
        matrix.printTime(7, 0, time, false);
      } else {
        matrix.printTime(7, 0, session->getLastLapMs(), false);
      }
    } else {
      matrix.printTime(7, 0, 0, false);
    }
    char lapsStr[2];
    // uint8_t stripes = (laps % 40) / 10;
    laps = laps % 10; // used to be 100
    if(laps < 10) {
      sprintf(lapsStr, "0%i", laps);
    } else {
      sprintf(lapsStr, "%i", laps);
    }
    int x = 0;
    if(lapsStr[0] != '0') {
      x = matrix.print(lapsStr[0], x, 0, CRGB::Yellow, FONT_SIZE_SMALL + FONT_SETTINGS_DEFAULT);
    }
    matrix.print(lapsStr[1], x, 0, CRGB::Yellow, FONT_SIZE_SMALL + FONT_SETTINGS_DEFAULT);
    // for (size_t i = 0; i < stripes; i++) {
    //   matrix.line(i, 0, i, 3, CRGB::Yellow);
    // }
    
  } else {
    for (int i = 0; i < min(double(getLEDCount()), millis() / 100.0 - 7); i++) {
      // Serial.println(analogRead(PIN_LASER));
      if(isTriggered() && millis() > 2000) {
        leds[i] = CRGB::White;
      } else {
        leds[i] = CRGB(30,0,20);
      }
    }
  }

  // debug
  freeHeapText->setValue(ESP.getFreeHeap());
  heapSizeText->setValue(ESP.getHeapSize());
  laserValue->setValue(digitalRead(PIN_LASER));

  // handle brightness
  float displayCurrent = predictLEDCurrentDraw();
  displayCurrentText->setValue(displayCurrent);
  float displayBrightness = MAX_CONTINUOUS_AMPS / displayCurrent * (displayBrightnessInput->getValue() / 100.0 * 255.0);
  if(displayBrightness > 255) displayBrightness = 255;
  displayCurrentAfterScaleText->setValue(displayCurrent * (displayBrightness / 255.0));
  static float lastDisplayBrightness = 0;
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