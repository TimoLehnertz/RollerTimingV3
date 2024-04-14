/**
 * @file main.cpp
 * @author Timo Lehnertz
 * @brief Main class for the Roller timing sytem
 * @version 3.1
 * @date 2023-07-10
 *
 * @copyright Copyright (c) 2024
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
#include <DoubleLinkedList.h>
#include <driver/adc.h>
#include <heltec.h>

void testMillionTriggers();

// void handleBattery() {
//   float voltageDividerMeasured = analogRead(PIN_VBAT) / 4095.0 * 3.3;
//   float vBatUnfiltered = voltageDividerMeasured * 3.8 + DIODE_VOLTAGE_DROP; // measured * scaling + diode
//   float percentLPF = 0.01;
//   vBat = vBatUnfiltered;
//   batPercent = (vBat - batEmpty) / (batFull - batEmpty) * 100; // vBat to bat%
//   // if(abs(lastBatPercent - batPercent) > 30) {
//   //   percentLPF = 1;
//   // }
//   batPercent = (percentLPF * batPercent) + (1.0 - percentLPF) * lastBatPercent; // Low pass filter
//   if(batPercent < 0) batPercent = 0;
//   if(batPercent > 100) batPercent = 100;
//   lastBatPercent = batPercent;
//   vBatText->setValue(vBat);
//   vBatMeasured->setValue(voltageDividerMeasured);
// }

void trigger()
{
  triggerCount++;
  lastTriggerMs = millis();
}

void setup()
{
  Serial.begin(115200);
  // simple
  Serial.printf("Roller timing v%s\n", VERSION);
  pinMode(PIN_ROTARY_GND, OUTPUT);
  digitalWrite(PIN_ROTARY_GND, LOW);
  beginSounds();
  pinMode(PIN_LED_WHITE, OUTPUT);
  pinMode(PIN_VBAT, INPUT);
  pinMode(PIN_LASER, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_LASER), trigger, FALLING);
  pinMode(PIN_BUZZER_GND, OUTPUT);
  digitalWrite(PIN_BUZZER_GND, LOW);
  pinMode(PIN_GND_1, OUTPUT);
  digitalWrite(PIN_GND_1, LOW);
  // complex without dependencies
  spiffsLogic.begin();

  pinMode(PIN_ROTARY_3V3, OUTPUT);
  digitalWrite(PIN_ROTARY_3V3, HIGH);
  Rotary.begin();
  beginLEDDisplay();
  beginRadio();
  beginLCDDisplay(); // needed for status variables
  // complex with dependencies
  beginMasterSlaveLogic(); // depends on beginLCDDisplay
  beginPreferences();      // depends on beginLCDDisplay
  // trigger changes
  initStationDisplay();
  beginWiFi();

  pinMode(1, INPUT);

  if (isDisplaySelect->getValue() && !spiffsLogic.isVersionMatch())
  {
    uiManager.popup("Update Spiffs now!");
  }

  Serial.println("Setup complete");
  /**
   * (Tests)
   */
  // testMillionTriggers();
  // Serial.println("formatting Spiffs");
  // bool succsess = SPIFFS.format();
  // if(succsess) {
  //   Serial.println("Spiffs formatted");
  // }
}

void handleTriggers()
{
  if (isDisplaySelect->getValue())
    return;
  static timeMs_t lastTimeTriggeredMs = 0;
  static timeMs_t lastTriggerCount = 0;
  if (lastTriggerCount != triggerCount)
  {
    lastTriggerCount = triggerCount;
    if (lastTriggerMs - lastTimeTriggeredMs < minDelayInput->getValue() * 1000 && lastTimeTriggeredMs != 0)
    {
      return;
    }
    lastTimeTriggeredMs = millis();
    EasyBuzzer.beep(3800, 20, 100, 1, 100, 1);
    if (!isDisplaySelect->getValue())
    {
      slaveTrigger(lastTriggerMs, stationTypeSelect->getValue(), uint16_t(distFromStartInput->getValue() * 1000.0));
    }
  }
}

/**
 * Its probably a good idea to remove file writing in TrainingsSession::addTrigger()
 */
void testMillionTriggers()
{
  int triggerType = STATION_TRIGGER_TYPE_START_FINISH; // 0
  uint64_t i = 0;
  while (true)
  {
    Trigger testTrigger;
    testTrigger.timeMs = millis() * 1000; // scaling up to simulate time passing by fast
    testTrigger.millimeters = 65500;
    testTrigger.triggerType = triggerType;
    triggerType++;
    if (triggerType > STATION_TRIGGER_TYPE_MAX)
    {
      triggerType = STATION_TRIGGER_TYPE_START_FINISH; // 0
    }
    masterTrigger(testTrigger);
    loop();
    i++;
    if (i % 100 == 0)
    {
      Serial.printf("%i Triggers tested\n", i);
    }
  }
}

void loop()
{
  /**
   * normal loop code
   */
  handleTriggers();
  handleRadioReceive();
  handleRadioSend();
  handleMasterSlaveLogic();
  handleRotary();
  uiManager.handle();
  handleLEDS();
  // handleBattery();
  handleSounds();
  handleStartgun();
  handleWiFi();
  EasyBuzzer.update();
  loops++;
  if (millis() - lastHzMeasuredMs > 1000)
  {
    loopHz = loops;
    hzText->setValue(loopHz);
    lastHzMeasuredMs = millis();
    loops = 0;
  }

  // adc1_config_width(ADC_WIDTH_BIT_12);
  // adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_0);
  // int val = adc1_get_raw(ADC1_CHANNEL_0);

  // float vbat = 100.0 / (100.0+390.0) * analogRead(1);
  // // float vbat = 100.0 / (100.0+390.0) * val;
  // Serial.printf("vBat: %f, val: %i\n", vbat, analogRead(1));
  // vBat = vbat;
  // float vbat = 100 / (100+390) * VADC_IN1;
}