/**
 * @file startgun.h
 * @author Timo Lehnertz
 * @brief Start gun logic for all hardware
 * @version 0.1
 * @date 2023-07-19
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once
#include <Arduino.h>
#include <Global.h>
#include <Sound.h>
#include <MasterSlaveLogic.h>

bool startgunStarted = false;

timeMs_t startgunInPositionMs = 0;
timeMs_t startgunSetMs = 0;
timeMs_t startgunGoMs = 0;

uint8_t startgunPhase = 0;

timeMs_t startgunLastBeep = 0;

timeMs_t startgunClearPopup = INT32_MAX;

double randomDouble(double minRand, double maxRand) {
    return minRand + (maxRand - minRand) * random(100000) / 100000.0;
}

void triggerStartGun() {
    for (size_t i = 0; i < 10; i++) {
        Serial.println(randomDouble(0, 1));
    }
    if(startgunStarted) return;
    startgunPhase = 1;
    startgunInPositionMs = millis() + inPositionMinDelay->getValue() * 1000 + (inPositionMaxDelay->getValue() - inPositionMinDelay->getValue()) * 1000.0 * randomDouble(0, 1);
    startgunSetMs = startgunInPositionMs + setMinDelay->getValue() * 1000 + (setMaxDelay->getValue() - setMinDelay->getValue()) * 1000.0 * randomDouble(0, 1);
    startgunGoMs = startgunSetMs + goMinDelay->getValue() * 1000 + (goMaxDelay->getValue() - goMinDelay->getValue()) * 1000.0 * randomDouble(0, 1);
    // startgunStarted = true;
    // playSoundStartgunIdle();
    uiManager.popup("Go to the start!");
    Serial.println(startgunInPositionMs);
    Serial.println(startgunSetMs);
    Serial.println(startgunGoMs);
}

void handleStartgun() {
    if(!startgunPhase) return;
    switch(startgunPhase) {
        case 1: {
            if(millis() - startgunLastBeep > 1000 && startgunInPositionMs - millis() > 3000) {
                playSoundStartgunIdle();
                startgunLastBeep = millis();
            }
            if(millis() > startgunInPositionMs) {
                uiManager.popup("In position!");
                playSoundStartgunInPosition();
                startgunPhase++;
            }
            break;
        }
        case 2: {
            if(millis() > startgunSetMs) {
                uiManager.popup("Set!");
                playSoundStartgunSet();
                startgunPhase++;
            }
            break;
        }
        case 3: {
            if(millis() > startgunGoMs) {
                uiManager.popup("Go!");
                startgunClearPopup = millis() + 3000;
                playSoundStartgunGo();
                startgunPhase = 0;
                startgunStarted = false;
                if(masterSlave.isMaster()) {
                    masterTrigger(millis(), 0, STATION_TRIGGER_TYPE_START); // 0 millimeters
                } else {
                    slaveTrigger(millis(), STATION_TRIGGER_TYPE_START);
                }
            }
            break;
        }
    }
    if(millis() > startgunClearPopup) {
        uiManager.popup(nullptr);
        startgunClearPopup = UINT32_MAX;
    }
}