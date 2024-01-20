/**
 * @file MasterSlaveLogic.h
 * @author Timo Lehnertz
 * @brief File for managing all high level comunications
 * @version 0.1
 * @date 2023-07-11
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once
#include <Global.h>
#include <WiFiLogic.h>
#include <Sound.h>
#include <GuiLogic.h>
#include <radio.h>
#include <DoubleLinkedList.h>

#define MASTER_TIMEOUT_MS 11000

void guiRemoveConnection(uint8_t address);
void guiSetConnection(uint8_t address, int millimeters, uint8_t lq, uint8_t stationType);

DoubleLinkedList<Trigger> slaveTriggers = DoubleLinkedList<Trigger>(); // for slaves

/**
 * Slave variables
 */
bool timeSynced = false;
timeMs_t lastTimeSyncMs = 0;
timeMs_t timeSyncOffset = 0;
timeMs_t nextSlaveTriggerSend = 0;
timeMs_t nextTriggerSend = 0;
bool masterConnected = false;


/**
 * Master variabled
 */
timeMs_t lastTimeSync = 0;

void sendTrigger(Trigger& trigger) {
    Serial.println("Sending trigger");
    sceduleSend((uint8_t*) &trigger, sizeof(Trigger));
}

void sendTimeSync() {
    Serial.println("Sending time sync");
    sceduleTimeSync();
}

bool isTimeSynced() {
    return timeSynced;
}

timeMs_t localTimeToMasterTime(timeMs_t localTimeMs) {
    return localTimeMs + timeSyncOffset;
}

struct TimeSyncMasterToSlave {
    timeMs_t currentTimeMs;
};

void beginMasterSlaveLogic() {

}

void slaveTrigger(timeMs_t atMs, uint8_t triggerType, uint16_t millimeters) {
    Trigger trigger = Trigger { atMs, millimeters, triggerType };
    trigger.timeMs += timeSyncOffset;
    slaveTriggers.pushBack(trigger);
    Serial.printf("Slave trigger #%i, triggerType: %i, millimeters: %i\n", slaveTriggers.getSize(), triggerType, millimeters);
}

void radioReceived(const uint8_t* byteArr, size_t size) {
    Serial.println("radioReceived");
    if(isDisplaySelect->getValue()) { // master
        if(size == sizeof(Trigger)) {
            if(lastTimeSync == 0) {
                return; // cant be synced yet
            }
            Trigger trigger = *((Trigger*) byteArr);
            timeMs_t timeVariance = abs(trigger.timeMs - timeMs_t(millis()));
            if(timeVariance < 15000) {
                if(!spiffsLogic.triggerInCache(trigger)) {
                    masterTrigger(trigger);
                    Serial.printf("Received trigger at %ims (time of receive: %ims)\n", trigger.timeMs, millis());
                } else {
                    Serial.println("Received already existing trigger");
                }
            } else {
                Serial.printf("Received trigger that was off by %ims. skipping", timeVariance);
            }
            sendTrigger(trigger); // copy that
        } else {
            Serial.printf("not trigger size: %i!=%i\n", size, sizeof(Trigger));
        }
    } else { // slave
        if(size == sizeof(Trigger)) { // trigger copy
            Trigger trigger = *((Trigger*) byteArr);
            if(slaveTriggers.getSize() > 0 && slaveTriggers.getFirst() == trigger) {
                Serial.println("Master got my trigger");
                slaveTriggers.removeIndex(0);
            }
        } else if(size == sizeof(uint32_t)) { // time sync
            if(slaveTriggers.getSize() > 0 && timeSynced) {
                Serial.println("Skipped time sync");
                return; // only allow time syncs when there are no triggers floating arround
            }
            for (auto &&slaveTrigger : slaveTriggers) { // restoring to local time
                slaveTrigger.timeMs -= timeSyncOffset;
            }
            uint32_t masterTime = *((uint32_t*) byteArr);
            masterTime += radio.getTimeOnAir(sizeof(uint32_t)) / 1000;
            long timeSyncOffsetBefore = timeSyncOffset;
            timeSyncOffset = timeMs_t(masterTime) - timeMs_t(millis());
            int32_t variance = timeSyncOffsetBefore - long(timeSyncOffset);
            Serial.printf("Synced time with master. offset: %i, variance: %i\n", timeSyncOffset, variance);
            if(timeSynced && abs(variance) > 1000) { // time was synced before
                Serial.println("Time sync variance was too big. Assume master has rebooted. Deleting qued triggers");
                playSoundNewConnection();
                slaveTriggers.clear();
            }
            for (auto &&slaveTrigger : slaveTriggers) { // applying master time
                slaveTrigger.timeMs += timeSyncOffset;
            }
            timeSynced = true;

            lastTimeSyncMs = millis();
            if(!masterConnected) {
                playSoundNewConnection();
            }
            masterConnected = true;
        }
    }
}

void handleMasterSlaveLogic() {
    if(isDisplaySelect->getValue()) { // master
        if(millis() - lastTimeSync > 5000) {
            sendTimeSync();
            lastTimeSync = millis();
        }
    } else { // slave
        if(timeSynced && slaveTriggers.getSize() > 0 && millis() > nextTriggerSend) {
            Serial.printf("sceduled triggers: %i\n", slaveTriggers.getSize());
            Trigger& first = slaveTriggers.getFirst();
            if(first.timeMs < 0) {
                Serial.println("removing negative trigger");
                slaveTriggers.removeIndex(0);
                return;
            }
            sendTrigger(first);
            timeMs_t triggerTimeout = random(3, 6) * (radio.getTimeOnAir(sizeof(Trigger)) / 1000 + 10);
            nextTriggerSend = millis() + triggerTimeout;
            Serial.printf("next trigger timeout: %ims\n", triggerTimeout);
        }
        if(masterConnected && long(millis()) - long(lastTimeSyncMs) > MASTER_TIMEOUT_MS) {
            masterConnected = false;
            Serial.println("Master disconnected");
            playSoundLostConnection();
        }
    }
}