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

#define TRAININGS_MODE_NORMAL 0
#define TRAININGS_MODE_TARGET 1

#define POWER_SAVING_MODE_OFF 0
#define POWER_SAVING_MODE_MEDIUM 1
#define POWER_SAVING_MODE_HIGH 2

#define MASTER_FRAMES 5
#define SLAVE_FRAMES 2

TextItem* connectionItems[MAX_CONNECTIONS];
char* connectionItemsTexts[MAX_CONNECTIONS];

void beginLEDDisplay();
void trainingsModeChanged();

int getLEDCount() {
  return displayStation ? NUM_LEDS_DISPLAY : NUM_LEDS_LASER;
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
  trainingsModeSelect->setHidden(!isMasterCB->isChecked());
  writePreferences();
}

void isDisplayChanged() {
  if(displayStation) {

  } else {

  }
  // only on displays
  displayBrightnessInput->setHidden(!displayStation);
  displayTimeInput->setHidden(!displayStation);
  powerSavingSelect->setHidden(!displayStation);
  // only on lasers
  stationTypeSelect->setHidden(displayStation);
  minDelayInput->setHidden(displayStation);
}

void guiSetConnection(uint8_t address, int millimeters, uint8_t lq) {
  if(address >= MAX_CONNECTIONS) return;
  if(connectionItemsTexts[address] == nullptr) {
    connectionItemsTexts[address] = new char[35];
  }
  if(millimeters >= 0) {
    sprintf(connectionItemsTexts[address], "#%i at %.1fm (Lq %i%)", address, millimeters / 1000.0, lq);
  } else {
    sprintf(connectionItemsTexts[address], "#%i (Lq %i%)", address, lq);
  }
  if(connectionItems[address] == nullptr) {
    connectionItems[address] = new TextItem(connectionItemsTexts[address], true);
    connectionsMenuMaster->addItem(connectionItems[address]);
  }
}

void guiRemoveConnection(uint8_t address) {
  if(address >= MAX_CONNECTIONS) return;
  connectionsMenuMaster->removeItem(connectionItems[address]);
  delete connectionItems[address];
  delete connectionItemsTexts[address];
  connectionItems[address] = nullptr;
  connectionItemsTexts[address] = nullptr;
}

void msOverlay(ScreenDisplay *display, DisplayUiState* state) {
  display->setColor(BLACK);
  display->fillRect(0, 0, 128, 13);
  display->setColor(WHITE);
  display->fillRect(7, 13, 114, 1);

  char strConnection[20];
  if(masterSlave.isMaster()) {
    size_t connections = masterSlave.getConnectedCount();
    if(connections == 0) {
      sprintf(strConnection, "No connections");
    } else {
      sprintf(strConnection, "%i connections (%i%)", connections, masterSlave.getConnectionByIndex(0)->lq);
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

// void updateViewer() {
//   TrainingsSession* session = spiffsLogic.getCurrentSession();
//   if(!session) return;
//   for (size_t i = 0; i < viewerMenu->getItemCount(); i++) {
//     MenuItem* item = viewerMenu->getItem(i);
//     viewerMenu->removeItem(item);
//     delete item;
//   }
  
//   for (size_t i = 0; i < session->getTriggerCount(); i++) {
//     Trigger t = session->getTrigger(i);
//     char triggerText[20];

//     char hStr[10] = "\0";
//     char mStr[3]  = "\0";
//     char sStr[3];
//     char msStr[4];
//     LedMatrix::timeToStr(ms, hStr, mStr, sStr, msStr, oneMsDigit);
//     sprintf(triggerText, "#%i %s", i, timeStr);
//     viewerMenu->addItem(new TextItem());
//   }
// }

void trainingsModeChanged() {
  Serial.println("trainingsMode changed");
  targetTimeSubMenu->setHidden(trainingsModeSelect->getValue() != TRAININGS_MODE_TARGET);
}

void stationTypeChanged() {
  distFromStartInput->setHidden(stationTypeSelect->getValue() != STATION_TRIGGER_TYPE_CHECKPOINT);
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
  uidText = new NumberField("Uid", "", 1, 0, UINT16_MAX, 0);
  uidText->setEditable(false);
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

  // All menu inits
  Menu* systemSettingsMenu = new Menu();
  Menu* setupMenu = new Menu();
  Menu* debugMenu = new Menu();
  Menu* menuFactoryReset = new Menu("No");
  connectionsMenuMaster = new Menu();
  connectionsMenuSlave = new Menu();
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
      menuFactoryReset->addItem(new TextItem("Power switch must me on", true));
      menuFactoryReset->addItem(new Button("Yes", factoryReset));

    systemSettingsMenu->addItem(showAdvancedCB);
    systemSettingsMenu->addItem(advancedText);
    systemSettingsMenu->addItem(isMasterCB);
    systemSettingsMenu->addItem(debugSubMenu);

      debugMenu->addItem(new TextItem("Info for nerds"));
      debugMenu->addItem(displayCurrentText);
      debugMenu->addItem(displayCurrentAfterScaleText);
      debugMenu->addItem(vBatMeasured);
      debugMenu->addItem(vBatText);
      debugMenu->addItem(hzText);
      debugMenu->addItem(uidText);
      debugMenu->addItem(new TextItem("Heap memory"));
      debugMenu->addItem(freeHeapText);
      debugMenu->addItem(heapSizeText);

  setupMenu->addItem(new SubMenu("Info", infoMenu));

    infoMenu->addItem(new TextItem("Roller timing", true));
    infoMenu->addItem(new TextItem(VERSION, true));
    infoMenu->addItem(new TextItem("www.roller-results.com", true));

  viewerMenu->addItem(new TextItem("Viewer"));


  connectionsMenuMaster->addItem(new TextItem("Master"));

  connectionsMenuSlave->addItem(new TextItem("Slave"));

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

uint32_t lastLEDUpdate = 0;

void handleLEDS() {
  if(millis() - lastLEDUpdate < 1000.0 / 30.0) return;
  lastLEDUpdate = millis();
  uidText->setValue(getUid());
  FastLED.clear();
  if(displayStation) {
    TrainingsSession* session = spiffsLogic.getCurrentSession();
    size_t laps = 0;
    if(session && session->getTriggerCount() > 0) {
      laps = session->getLapsCount();
      if(session->getTriggerCount() == 1 || session->getTimeSinceLastTrigger() > displayTimeInput->getValue() * 1000) {
        uint32_t time = session->getTimeSinceLastTrigger();
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
    for (int i = 0; i < min(getLEDCount(), millis() / 100.0 - 7); i++) {
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

  // handle brightness
  float displayCurrent = predictLEDCurrentDraw();
  displayCurrentText->setValue(displayCurrent);
  float displayBrightness = MAX_CONTINUOUS_AMPS / displayCurrent * (displayBrightnessInput->getValue() / 100.0 * 255.0);
  if(displayBrightness > 255) displayBrightness = 255;
  displayCurrentAfterScaleText->setValue(displayCurrent * (displayBrightness / 255.0));
  static float lastDisplayBrightness = 0;
  float brightnessLPF = 0.1;
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