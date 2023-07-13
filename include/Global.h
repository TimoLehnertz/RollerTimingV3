/**
 * @file Global.h
 * @author Timo Lehnertz
 * @brief Container for all variables that will be available globally
 * @version 0.1
 * @date 2023-07-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once

#include <Arduino.h>
#include <RadioLib.h>
// from arduino ide in C:\Users\timol\AppData\Local\Arduino15\packages\Heltec-esp32\hardware\esp32\0.0.7\libraries
#include "HT_SSD1306Wire.h" // legacy include: `#include "SSD1306.h"`
#include "HT_DisplayUi.h"
#include <RotaryEncoder.h>
#include <MasterSlave.h>
#include <gui.h>
#include <FastLED.h>
#include <LedMatrix.h>
#include <definitions.h>
#include <SPIFFSLogic.h>

SPIFFSLogic spiffsLogic = SPIFFSLogic();

uint32_t timeForSize(uint8_t size);
MasterSlave masterSlave(false, timeForSize);
SSD1306Wire  display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr , freq , i2c group , resolution , rst
SX1262 radio = new Module(LoRa_nss, LoRa_dio1, LoRa_nrst, LoRa_busy);

DisplayUi ui( &display );

UIManager uiManager(&ui);

// rotary
int Counter = 0, LastCount = 0; //uneeded just for test
void RotaryChanged(); //we need to declare the func above so Rotary goes to the one below
RotaryEncoder Rotary(&RotaryChanged, PIN_ROTARY_DT, PIN_ROTARY_CLK, PIN_ROTARY_SW); // Pins 2 (DT), 3 (CLK), 4 (SW)

CRGB leds[NUM_LEDS_DISPLAY];
LedMatrix matrix;

volatile bool receivedFlag = false;
// bool sendingFlag = false;
uint64_t sendingUntilUs = 0;

size_t loops = 0;
uint32_t lastHzMeasuredMs = 0;
uint32_t loopHz = 0;

/**
 * Battery stuff
 */
float vBatLPF = 0.0005;

float batFull = 4.1;
float batEmpty = 3.4; // safety margin

float batPercent = 50;
float lastBatPercent = 50;
float vBatPercentLPF = 0.1;
float vBat = 3.8;

/**
 * Gui variables
 */
MenuItem* menuSetupItems[4];
NumberField* distFromStartInput;
NumberField* minDelayInput;
NumberField* displayTimeInput;
NumberField* displayBrightnessInput;
CheckBox* isDisplayCheckbox;
CheckBox* isMasterCB;

// Debug items
MenuItem* debugMenuItems[5];
NumberField* displayCurrentText;
NumberField* displayCurrentAfterScaleText;
NumberField* vBatMeasured;
NumberField* vBatText;
NumberField* hzText;
NumberField* uidText;

Menu* connectionsMenuMaster;
Menu* connectionsMenuSlave;

FrameSection* connectionsFrameSection;

volatile uint32_t triggerCount = 0;
volatile uint32_t lastTriggerMs;

void msOverlay(ScreenDisplay *display, DisplayUiState* state);
OverlayCallback overlayCallbacks[] = { msOverlay };
size_t overlaysCount = 1;

/**
 * Some global functions
 */
ICACHE_RAM_ATTR void setFlag(void);

bool isDisplay() {
  return isDisplayCheckbox->isChecked();
}

bool isTriggered() {
  return !digitalRead(PIN_LASER);
}

void masterTrigger(uint32_t masterTimeMs, uint16_t uid) {
  spiffsLogic.addTrigger(masterTimeMs, uid);
}

/**
 * Some classes
 */
#ifndef TRIGGER_DEFINED
#define TRIGGER_DEFINED
struct Trigger {
  uint32_t timeMs; // overflows after 50 days
  uint16_t macAddress;
};
#endif