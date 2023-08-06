/**
 * @file Sound.h
 * @author Timo Lehnertz
 * @brief All sound related logic
 * @version 0.1
 * @date 2023-07-19
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once
#include <Arduino.h>
#include <Global.h>

struct Beep {

    Beep() : frequency(0), onDuration(0), offDuration(0), beeps(0), pauseAfterBeep(0) {}
    Beep(timeMs_t pauseAfterBeep, unsigned int frequency, unsigned int onDuration) : frequency(frequency), onDuration(onDuration), offDuration(0), beeps(1), pauseAfterBeep(pauseAfterBeep) {}
    Beep(timeMs_t pauseAfterBeep, unsigned int frequency, unsigned int onDuration, unsigned int offDuration, byte beeps) : frequency(frequency), onDuration(onDuration), offDuration(offDuration), beeps(beeps), pauseAfterBeep(pauseAfterBeep) {}

    unsigned int frequency;
    unsigned int onDuration;
    unsigned int offDuration;
    byte beeps;

    timeMs_t pauseAfterBeep;

    timeMs_t getFullDuration() {
        return (onDuration + offDuration) * beeps - offDuration + pauseAfterBeep;
    }

    timeMs_t beep() {
        // EasyBuzzer.beep(
        //     4500,		// Frequency in hertz(HZ). 
        //     20, 	// On Duration in milliseconds(ms).
        //     100, 	// Off Duration in milliseconds(ms).
        //     1, 			// The number of beeps per cycle.
        //     100, 	// Pause duration.
        //     1 		// The number of cycle.
        // );
        EasyBuzzer.beep(frequency, onDuration, offDuration, beeps, 100, 1);
        return millis() + getFullDuration();
    }
};

struct Sound {
    Sound() : currentBeep(0), beepsSize(0) {}
    Beep beeps[10];
    uint8_t currentBeep;
    size_t beepsSize = 0;

    void addBeep(Beep beep) {
        beeps[beepsSize++] = beep;
    }
};

Sound currentSound;
timeMs_t nextBeep = UINT32_MAX;

void playSound(Sound sound) {
    if(sound.beepsSize == 0) return;
    currentSound = sound;
    nextBeep = millis();
}

void beginSounds() {
    EasyBuzzer.setPin(PIN_BUZZER_PLUS);
}

void handleSounds() {
    if(millis() >= nextBeep) {
        if(currentSound.beepsSize > currentSound.currentBeep) {
            nextBeep = currentSound.beeps[currentSound.currentBeep++].beep();
        }
    }
}

void playSoundStartgunIdle() {
    Sound sound = Sound();
    sound.addBeep(Beep(50, 4000, 10));
    playSound(sound);
    // Serial.println("playSoundStartgunIdle");
    // EasyBuzzer.beep(4000, 50, 10, 1, 100, 1);
}

void playSoundStartgunInPosition() {
    Sound sound = Sound();
    sound.addBeep(Beep(50, 5000, 100, 200, 3));
    playSound(sound);
}

void playSoundStartgunSet() {
    Sound sound = Sound();
    sound.addBeep(Beep(50, 5000, 500, 200, 2));
    playSound(sound);
}

void playSoundStartgunGo() {
    Sound sound = Sound();
    sound.addBeep(Beep(50, 5500, 500));
    playSound(sound);
}

void playSoundNewConnection() {
    Sound sound = Sound();
    sound.addBeep(Beep(50, 4000, 100));
    sound.addBeep(Beep(50, 8000, 100));
    playSound(sound);
}

void playSoundLostConnection() {
    Sound sound = Sound();
    sound.addBeep(Beep(50, 8000, 100));
    sound.addBeep(Beep(50, 4000, 100));
    playSound(sound);
}

void playSoundBootUp() {
    Sound sound = Sound();
    sound.addBeep(Beep(50, 6000, 200));
    sound.addBeep(Beep(50, 8000, 200));
    sound.addBeep(Beep(50, 9000, 200));
    playSound(sound);
}