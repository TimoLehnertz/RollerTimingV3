#pragma once
#include <Arduino.h>
#include <Global.h>
#include <LinkedList.h>

struct Beep {

    Beep() : frequency(0), onDuration(0), offDuration(0), beeps(0), pauseAfterBeep(0) {}
    Beep(uint32_t pauseAfterBeep, unsigned int frequency, unsigned int onDuration) : frequency(frequency), onDuration(onDuration), offDuration(0), beeps(1), pauseAfterBeep(pauseAfterBeep) {}
    Beep(uint32_t pauseAfterBeep, unsigned int frequency, unsigned int onDuration, unsigned int offDuration, byte beeps) : frequency(frequency), onDuration(onDuration), offDuration(offDuration), beeps(beeps), pauseAfterBeep(pauseAfterBeep) {}

    unsigned int frequency;
    unsigned int onDuration;
    unsigned int offDuration;
    byte beeps;

    uint32_t pauseAfterBeep;

    uint32_t getFullDuration() {
        return (onDuration + offDuration) * beeps - offDuration + pauseAfterBeep;
    }

    uint32_t beep() {
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
uint32_t nextBeep = UINT32_MAX;

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