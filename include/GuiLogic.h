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

#define POWER_SAVING_MODE_OFF 0
#define POWER_SAVING_MODE_MEDIUM 1
#define POWER_SAVING_MODE_HIGH 2

TextItem* connectionItems[MAX_CONNECTIONS];
char* connectionItemsTexts[MAX_CONNECTIONS];

void beginLEDDisplay();

int getLEDCount() {
  return isDisplay() ? NUM_LEDS_DISPLAY : NUM_LEDS_LASER;
}

uint16_t pixelConverter(uint16_t x, uint16_t y) {
  y = 7 - y;
  return (8 * 31) - x * 8 + (x % 2 == 0 ? (7 - y) : y);
}

void isMasterChanged();

void isDisplayChanged() {
  isMasterCB->setChecked(isDisplayCheckbox->isChecked());
  isMasterChanged();
  writePreferences();
}

void powerSavingChanged() {
  switch(powerSavingSelect->getValue()) {
    case POWER_SAVING_MODE_OFF: {
      masterSlave.setComunicationDelay(0);
      radio.setOutputPower(22); // full power (150mw)
      break;
    }
    case POWER_SAVING_MODE_MEDIUM: {
      masterSlave.setComunicationDelay(100);
      radio.setOutputPower(19); // 80mw
      break;
    }
    case POWER_SAVING_MODE_HIGH: {
      masterSlave.setComunicationDelay(200);
      radio.setOutputPower(9); // 8 mw
      break;
    }
  }
}

void simpleInputChanged() {
  writePreferences();
}

void isMasterChanged() {
  Serial.println("isMasterChanged");
  masterSlave.setMaster(isMasterCB->isChecked());
  // distFromStartInput->setHidden(isMasterCB->isChecked());
  if(isMasterCB->isChecked()) {
    connectionsFrameSection->setMenu(connectionsMenuMaster);
    spiffsLogic.startNewSession();
  } else {
    spiffsLogic.endSession();
    connectionsFrameSection->setMenu(connectionsMenuSlave);
  }
  writePreferences();
}

void guiSetConnection(uint8_t address, int meters, uint8_t lq) {
  if(address >= MAX_CONNECTIONS) return;
  // return;
  connectionItemsTexts[address] = new char[40];
  if(meters >= 0) {
    sprintf(connectionItemsTexts[address], "#%i at %im (Link quality %i%)", address, meters, lq);
  } else {
    sprintf(connectionItemsTexts[address], "#%i (Link quality %i%)", address, lq);
  }
  if(connectionItems[address] == nullptr) {
    // connectionItems[address] = new TextItem(connectionItemsTexts[address], true);
    // connectionsMenuMaster->addItem(connectionItems[address]);
  }
}

void guiRemoveConnection(uint8_t address) {
  if(address >= MAX_CONNECTIONS) return;
  connectionsMenuMaster->removeItem(connectionItems[address]);
  delete connectionItems[address];
  delete connectionItemsTexts[address];
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
      sprintf(strConnection, "No connections");
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

void drawAdvancedSettings(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display->setFont(ArialMT_Plain_16);
  display->drawStringMaxWidth(x + 64, y + 32, 100, "Advanced settings");
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

void beginLCDDisplay() {
  distFromStartInput = new NumberField("Dist. from start", "m", 1, 0, 256, 1, 0, simpleInputChanged);
  minDelayInput = new NumberField("Min. delay", "s", 0.1, 0.5, 1000, 1, 0, simpleInputChanged);
  isDisplayCheckbox = new CheckBox("Is display", false, false, isDisplayChanged);
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
  displayBrightnessInput = new NumberField("Brightness", "%", 5, 5, 100, 0, 30, simpleInputChanged);
  displayTimeInput = new NumberField("Lap dispplay", "s", 0.5, 0.5, 100, 1, 3, simpleInputChanged);
  powerSavingSelect = new Select("Power saving", powerSavingChanged);
  powerSavingSelect->addOption("Off", "Off");
  powerSavingSelect->addOption("Medium", "Medium");
  powerSavingSelect->addOption("Energy saver", "High");

  // All menu inits
  Menu* systemSettingsMenu = new Menu();
  Menu* setupMenu = new Menu();
  Menu* debugMenu = new Menu();
  Menu* menuFactoryReset = new Menu("No");
  connectionsMenuMaster = new Menu();
  connectionsMenuSlave = new Menu();
  Menu* viewerMenu = new Menu();
  Menu* wifiMenu = new Menu();

  setupMenu->addItem(displayTimeInput);
  setupMenu->addItem(distFromStartInput);
  setupMenu->addItem(minDelayInput);
  setupMenu->addItem(displayBrightnessInput);
  setupMenu->addItem(new SubMenu("System settings", systemSettingsMenu));

    systemSettingsMenu->addItem(new TextItem("Advanced settings"));
    systemSettingsMenu->addItem(powerSavingSelect);
    systemSettingsMenu->addItem(isDisplayCheckbox);
    systemSettingsMenu->addItem(isMasterCB);
    systemSettingsMenu->addItem(new SubMenu("Debug", debugMenu));

      debugMenu->addItem(displayCurrentText);
      debugMenu->addItem(displayCurrentAfterScaleText);
      debugMenu->addItem(vBatMeasured);
      debugMenu->addItem(vBatText);
      debugMenu->addItem(hzText);
      debugMenu->addItem(uidText);

    systemSettingsMenu->addItem(new SubMenu("Factory reset", menuFactoryReset));

      menuFactoryReset->addItem(new TextItem("Are you sure?"));
      menuFactoryReset->addItem(new Button("Yes", factoryReset));

  connectionsMenuMaster->addItem(new TextItem("Server"));

  connectionsMenuSlave->addItem(new TextItem("Client"));

  FrameSection* frameSections = new FrameSection[4];
  frameSections[0] = FrameSection(drawSetup, setupMenu);
  frameSections[1] = FrameSection(drawConnections, connectionsMenuMaster);
  frameSections[2] = FrameSection(drawViewer, viewerMenu);
  frameSections[3] = FrameSection(drawFrameWiFi, wifiMenu);

  connectionsFrameSection = &frameSections[1];

  uiManager.begin(overlayCallbacks, overlaysCount, frameSections, 4);

  for (size_t i = 0; i < MAX_CONNECTIONS; i++) {
    connectionItems[i] = nullptr;
  }
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
  if(isDisplay()) {
    TrainingsSession* session = spiffsLogic.getCurrentSession();
    size_t laps = 0;
    if(session && session->getTriggerCount() > 0) {
      laps = session->getLapsCount();
      if(session->getTriggerCount() == 1 || session->getTimeSinceLastTrigger() > displayTimeInput->getValue() * 1000) {
        matrix.printTime(9, 0, session->getTimeSinceLastTrigger(), true);
      } else {
        matrix.printTime(9, 0, session->getLastLapMs(), false);
      }
    } else {
      matrix.printTime(9, 0, 0, false);
    }
    char lapsStr[2];
    // uint8_t stripes = (laps % 40) / 10;
    laps = laps % 100;
    if(laps < 10) {
      sprintf(lapsStr, "0%i", laps);
    } else {
      sprintf(lapsStr, "%i", laps);
    }
    int x = 0;
    if(lapsStr[0] != '0') {
      x = matrix.print(lapsStr[0], x, 3, CRGB::Yellow, FONT_SIZE_SMALL + FONT_SETTINGS_DEFAULT);
    }
    matrix.print(lapsStr[1], x, 3, CRGB::White, FONT_SIZE_SMALL + FONT_SETTINGS_DEFAULT);
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

  // handle brightness
  float displayCurrent = predictLEDCurrentDraw();
  displayCurrentText->setValue(displayCurrent);
  int displayBrightness = MAX_CONTINUOUS_AMPS / displayCurrent * (displayBrightnessInput->getValue() / 100.0 * 255.0);
  if(displayBrightness > 255) displayBrightness = 255;
  displayCurrentAfterScaleText->setValue(displayCurrent * (displayBrightness / 255.0));
  FastLED.setBrightness(displayBrightness);
  FastLED.show();
  if(millis() < 1000) {
    digitalWrite(PIN_LED_WHITE, millis() % 100 > 50);
  } else {
    digitalWrite(PIN_LED_WHITE, LOW);
  }
}