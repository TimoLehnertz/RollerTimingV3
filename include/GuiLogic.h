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

void beginLEDDisplay();

int getLEDCount() {
  return isDisplay() ? NUM_LEDS_DISPLAY : NUM_LEDS_LASER;
}

uint16_t pixelConverter(uint16_t x, uint16_t y) {
  y = 7 - y;
  return (8 * 31) - x * 8 + (x % 2 == 0 ? (7 - y) : y);
}

void isDisplayChanged() {
  beginLEDDisplay();
  writePreferences();
  isMasterCB->setChecked(isDisplayCheckbox->isChecked());
}

void isMasterChanged() {
  masterSlave.setMaster(isMasterCB->isChecked());
  if(isMasterCB->isChecked()) {
    connectionsFrameSection->setMenu(connectionsMenuMaster);
  } else {
    connectionsFrameSection->setMenu(connectionsMenuSlave);
  }
}

void msOverlay(ScreenDisplay *display, DisplayUiState* state) {
  display->setColor(BLACK);
  display->fillRect(0, 0, 128, 13);
  display->setColor(WHITE);
  display->fillRect(7, 13, 114, 1);
  char strBat[12];
  char strLaser[10];
  // sprintf(strBat, "%i%%", int(round(batPercent)));
  sprintf(strBat, "%i%%", int(round(batPercent / 10.0) * 10.0));
  sprintf(strLaser, "%i-%i", triggerCount, digitalRead(PIN_LASER));

  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  // if(vBat > 4.25) {
  //   display->drawString(128, 0, String("USB"));
  // } else if(millis() > 0) { // wait for bat readings to converge
    display->drawString(128, 0, String(strBat));
  // }
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0, 0, String(strLaser));
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
  distFromStartInput = new NumberField("Dist. from start", "m", 1, 0, 256, 1, 0, writePreferences);
  minDelayInput = new NumberField("Min. delay", "s", 0.1, 0.5, 1000, 1, 0, writePreferences);
  isDisplayCheckbox = new CheckBox("Is display", false, isDisplayChanged);
  isMasterCB = new CheckBox("Is master", false, isMasterChanged);

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

  // All menu inits
  Menu* systemSettingsMenu = new Menu();
  Menu* setupMenu = new Menu();
  Menu* debugMenu = new Menu();
  Menu* menuFactoryReset = new Menu();
  connectionsMenuMaster = new Menu();
  connectionsMenuSlave = new Menu();
  Menu* viewerMenu = new Menu();
  Menu* wifiMenu = new Menu();

  setupMenu->addItem(distFromStartInput);
  setupMenu->addItem(minDelayInput);
  setupMenu->addItem(new SubMenu("System settings", systemSettingsMenu));

    systemSettingsMenu->addItem(new TextItem("Advanced settings"));
    systemSettingsMenu->addItem(isDisplayCheckbox);
    systemSettingsMenu->addItem(isMasterCB);
    systemSettingsMenu->addItem(new SubMenu("Debug", debugMenu));

      debugMenu->addItem(displayCurrentText);
      debugMenu->addItem(displayCurrentAfterScaleText);
      debugMenu->addItem(vBatMeasured);
      debugMenu->addItem(vBatText);
      debugMenu->addItem(hzText);

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
}

void beginLEDDisplay() {
  FastLED.clearData();
  if(isDisplay()) {
    FastLED.addLeds<NEOPIXEL, PIN_WS2812b>(leds, NUM_LEDS_DISPLAY);
    matrix = LedMatrix(leds, NUM_LEDS_DISPLAY, pixelConverter);
  } else {
    FastLED.addLeds<NEOPIXEL, PIN_WS2812b>(leds, NUM_LEDS_LASER);
    matrix = LedMatrix();
  }
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

  if(isDisplay()) {
    FastLED.clear();
    char str[10];
    sprintf(str, "%.3f", millis() / 1000.0);
    matrix.printTime(0,0, millis(), true);
  }

  // handle brightness
  float displayCurrent = predictLEDCurrentDraw();
  displayCurrentText->setValue(displayCurrent);
  int displayBrightness = MAX_CONTINUOUS_AMPS / displayCurrent * 255;
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