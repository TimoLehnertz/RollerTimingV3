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
  lastTriggerUs = micros();
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_ROTARY_GND, OUTPUT);
  digitalWrite(PIN_ROTARY_GND, LOW);
  Rotary.begin();
  pinMode(PIN_LED_WHITE, OUTPUT);
  pinMode(PIN_VBAT, INPUT);
  pinMode(PIN_LASER, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_LASER), trigger, FALLING);
  pinMode(PIN_BUZZER_GND, OUTPUT);
  digitalWrite(PIN_BUZZER_GND, LOW);
  EasyBuzzer.setPin(PIN_BUZZER_PLUS);
  beginRadio();
  beginLCDDisplay();
  beginLEDDisplay();
  beginPreferences();
  SpiffsLogic.begin();
  beginMasterSlaveLogic();
  Serial.println("Setup complete");
}

/**
 * @return uint16_t the last 2 bytes of the mac hoping that those uniquely identify each chip
 */
uint16_t getUid() {
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  return *((uint16_t*)&mac[6]);
}

void handleTriggers() {
  static uint32_t lastTrigger = 0;
  static uint32_t lastTriggerCount = 0;
  if(lastTriggerCount != triggerCount) {
    lastTriggerCount = triggerCount;
    if(millis() - lastTrigger < minDelayInput->getValue()) {
      return;
    }
  }
}

void loop() {
  handleRadioReceive();
  handleradioSend();
  handleMasterSlaveLogic();
  uiManager.handle();
  handleTriggers();
  handleLEDS();
  handleRotary();
  handleBattery();
  EasyBuzzer.update();
  loops++;
  if(millis() - lastHzMeasuredMs > 1000) {
    loopHz = loops;
    hzText->setValue(loopHz);
    lastHzMeasuredMs = millis();
    loops = 0;
  }
}