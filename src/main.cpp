/**
 * @file main.cpp
 * @author Timo Lehnertz
 * @brief Main class for the Roller timing sytem
 * @version 0.1
 * @date 2023-07-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <Arduino.h>
#include <Global.h>
#include <EasyBuzzer.h>
#include <WiFi.h>
#include <definitions.h>
#include <GuiLogic.h>
#include <Storage.h>
#include <radio.h>
#include <rotary.h>
#include <MasterSlaveLogic.h>
#include <WiFiLogic.h>
#include <Sound.h>

void handleBattery() {
  float voltageDividerMeasured = analogRead(PIN_VBAT) / 4095.0 * 3.3;
  float vBatUnfiltered = voltageDividerMeasured * 3.8 + DIODE_VOLTAGE_DROP; // measured * scaling + diode
  float percentLPF = 0.01;
  vBat = vBatUnfiltered;
  batPercent = (vBat - batEmpty) / (batFull - batEmpty) * 100; // vBat to bat%
  // if(abs(lastBatPercent - batPercent) > 30) {
  //   percentLPF = 1;
  // }
  batPercent = (percentLPF * batPercent) + (1.0 - percentLPF) * lastBatPercent; // Low pass filter
  if(batPercent < 0) batPercent = 0;
  if(batPercent > 100) batPercent = 100;
  lastBatPercent = batPercent;
  vBatText->setValue(vBat);
  vBatMeasured->setValue(voltageDividerMeasured);
}

void trigger() {
  triggerCount++;
  lastTriggerMs = millis();
}

void setup() {
  Serial.begin(115200);
  // simple
  pinMode(PIN_ROTARY_GND, OUTPUT);
  digitalWrite(PIN_ROTARY_GND, LOW);
  beginSounds();
  pinMode(PIN_LED_WHITE, OUTPUT);
  pinMode(PIN_VBAT, INPUT);
  pinMode(PIN_LASER, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_LASER), trigger, FALLING);
  pinMode(PIN_BUZZER_GND, OUTPUT);
  digitalWrite(PIN_BUZZER_GND, LOW);
  // complex without dependencies
  spiffsLogic.begin();
  
  Rotary.begin();
  beginLEDDisplay();
  beginRadio();
  beginLCDDisplay(); // needed for status variables
  // complex with dependencies
  beginPreferences(); // depends on beginLCDDisplay
  beginMasterSlaveLogic(); // depends on beginLCDDisplay
  isMasterChanged();
  isDisplayChanged();
  Serial.println("Setup complete");
}


void handleTriggers() {
  if(displayStation) return;
  static uint32_t lastTimeTriggeredMs = 0;
  static uint32_t lastTriggerCount = 0;
  if(lastTriggerCount != triggerCount) {
    lastTriggerCount = triggerCount;
    if(lastTriggerMs - lastTimeTriggeredMs < minDelayInput->getValue() * 1000) {
      return;
    }
    lastTimeTriggeredMs = millis();
    EasyBuzzer.beep(3800, 20, 100, 1,  100, 1);
    if(isMasterCB->isChecked()) {

    } else {
      slaveTrigger(lastTriggerMs, stationTypeSelect->getValue());
    }
  }
}

void loop() {
  // if(millis() % 100 == 0)
  //   Serial.println(analogRead(PIN_LASER));
  handleTriggers();
  handleRadioReceive();
  handleradioSend();
  handleMasterSlaveLogic();
  uiManager.handle();
  handleLEDS();
  handleRotary();
  handleBattery();
  beginWiFi();
  handleSounds();
  handleStartgun();
  EasyBuzzer.update();
  loops++;
  if(millis() - lastHzMeasuredMs > 1000) {
    loopHz = loops;
    hzText->setValue(loopHz);
    lastHzMeasuredMs = millis();
    loops = 0;
  }
}