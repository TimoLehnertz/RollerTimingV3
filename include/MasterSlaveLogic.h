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
#include <LinkedList.h>
#include <WiFiLogic.h>
#include <Sound.h>

#define MAX_SLAVE_TRIGGERS 256

struct SlaveLaser {
    uint8_t address; // actual MasterSlave connection containing metadata
    uint16_t triggerIndex; // holds the index of the last received trigger
    uint16_t uid; // number identifying the device
    bool connected;
};

LinkedList<SlaveLaser*> slaveLasers = LinkedList<SlaveLaser*>(); // for masters
LinkedList<Trigger> slaveTriggers = LinkedList<Trigger>(); // for slaves

bool timeSynced = false;
uint32_t lastTimeSyncMs = 0;
int32_t timeSyncOffset = 0;

bool isTimeSynced() {
    return timeSynced;
}

uint32_t localTimeToMasterTime(uint32_t localTimeMs) {
    if(long(localTimeMs) + timeSyncOffset < 0) return 0;
    return localTimeMs + timeSyncOffset;
}

void addSlaveLaser(SlaveLaser* slaveLaser) {
    Serial.printf("Discovered new laser on address %i. uid: %i\n", slaveLaser->address, slaveLaser->uid);
    slaveLasers.add(slaveLaser);
}

SlaveLaser* getLaserByAddress(uint8_t address) {
    for (size_t i = 0; i < slaveLasers.size(); i++) {
        if(slaveLasers.get(i)->address == address) {
            return slaveLasers.get(i);
        }
    }
    return nullptr;
}

SlaveLaser* getLaserByUid(uint16_t uid) {
    for (size_t i = 0; i < slaveLasers.size(); i++) {
        if(slaveLasers.get(i)->uid == uid) {
            return slaveLasers.get(i);
        }
    }
    return nullptr;
}

void SlaveFoundMasterCallbackFunc(uint8_t ownAddress) {
    Serial.printf("Found master. New address: %i\n", ownAddress);
    playSoundNewConnection();
}

void SlaveLostMasterCallbackFunc() {
    Serial.println("Master disconnected");
    timeSynced = false;
    playSoundLostConnection();
}

void MasterGotNewConnectionCallbackFunc(Connection* connection) {
    Serial.printf("New connection on address %i\n", connection->address);
    playSoundNewConnection();
}

void SlaveDisconnectedCallbackFunc(uint8_t slaveAddress) {
    Serial.printf("Slave with address %i disconnected\n", slaveAddress);
    SlaveLaser* slaveLaser = getLaserByAddress(slaveAddress);
    if(!slaveLaser) return;
    slaveLaser->connected = false;
    playSoundLostConnection();
}

struct TriggersMsgMasterToSlave {
    uint16_t triggerIndex; // the trigger index
    uint16_t slaveUid;
};

struct TriggersMsgSlaveToMaster {

    uint16_t uid; // slave uid
    bool rebootedFlag;
    Trigger triggers[3]; // 0 - 3 triggers
    
    static uint8_t getSize(uint8_t triggerCount) {
        return sizeof(uint16_t) + sizeof(bool) + triggerCount* sizeof(Trigger);
    }

    static uint8_t getTriggerCount(uint8_t size) {
        return (size - sizeof(uint16_t) - sizeof(bool)) / sizeof(Trigger);
    }
};

void triggersMasterDataCallback(uint8_t address, uint8_t* data, uint8_t* dataSize) {
    SlaveLaser* slaveLaser = getLaserByAddress(address);
    uint16_t triggerIndex = 0;
    uint16_t slaveUid = 0;
    if(slaveLaser) {
        triggerIndex = slaveLaser->triggerIndex;
        slaveUid = slaveLaser->uid;
    }
    TriggersMsgMasterToSlave masterToSlave = TriggersMsgMasterToSlave{ triggerIndex, slaveUid };
    memcpy(data, &masterToSlave, sizeof(TriggersMsgMasterToSlave));
    *dataSize = sizeof(TriggersMsgMasterToSlave);
}

void triggersSlaveDataCallback(uint8_t* data, uint8_t dataSize, uint8_t* response, uint8_t* responseSize) {
    if(dataSize != sizeof(TriggersMsgMasterToSlave)) { // probably incorrect version
        return;
    }
    TriggersMsgMasterToSlave* masterToSlave = (TriggersMsgMasterToSlave*) data;
    TriggersMsgSlaveToMaster slaveToMaster;
    slaveToMaster.uid = getUid();
    slaveToMaster.rebootedFlag = false;
    uint8_t sendTriggers = 0;
    // Serial.printf("Master wants %i, i got %i\n", masterToSlave->triggerIndex, slaveTriggers.size());
    if(masterToSlave->slaveUid == getUid()) {
        if(!isTimeSynced()) {
            // dont send any triggers yet
        } else if(masterToSlave->triggerIndex > slaveTriggers.size() || masterToSlave->triggerIndex >= MAX_SLAVE_TRIGGERS) { // Slave must have rebooted
            Serial.println("I seem to have rebooted or have been overflowing");
            slaveTriggers.clear();
            slaveToMaster.rebootedFlag = true;
        } else {
            for (size_t i = masterToSlave->triggerIndex; i < min(slaveTriggers.size(), masterToSlave->triggerIndex + 3); i++) { // send max 3 triggers
                Trigger trigger = slaveTriggers.get(i);
                trigger.timeMs = localTimeToMasterTime(trigger.timeMs); // convert time
                slaveToMaster.triggers[sendTriggers++] = trigger;
            }
        }
    } else {
        Serial.printf("uid missmatch %i != %i\n", masterToSlave->slaveUid, getUid());
    }
    if(sendTriggers > 0) {
        Serial.printf("Send %i triggers\n", sendTriggers);
    }
    *responseSize = TriggersMsgSlaveToMaster::getSize(sendTriggers);
    memcpy(response, &slaveToMaster, *responseSize);
}

void triggersMasterReceiveCallback(uint8_t* data, uint8_t size, uint8_t slaveAddress) {
    // Serial.printf("slave data (%i) from %i\n", size, slaveAddress);
    if(size < TriggersMsgSlaveToMaster::getSize(0)) return;
    TriggersMsgSlaveToMaster* slaveToMaster = (TriggersMsgSlaveToMaster*) data;
    SlaveLaser* slaveLaser = getLaserByUid(slaveToMaster->uid);
    if(!slaveLaser) {
        slaveLaser = new SlaveLaser{ slaveAddress, 0, slaveToMaster->uid, true };
        addSlaveLaser(slaveLaser);
        return; // only add for now
    }
    slaveLaser->address = slaveAddress; // in case address changed
    if(slaveToMaster->rebootedFlag) {
        Serial.println("Slave has rebooted. Resetting triggerIndex");
        slaveLaser->triggerIndex = 0;
    }
    size_t triggerCount = TriggersMsgSlaveToMaster::getTriggerCount(size);
    for (size_t i = 0; i < triggerCount; i++) {
        slaveLaser->triggerIndex++;
        masterTrigger(slaveToMaster->triggers[i].timeMs, slaveToMaster->triggers[i].uid);
        Serial.printf("Received trigger from uid: %i at address %i, new triggerindex: %i\n", slaveToMaster->uid, slaveAddress, slaveLaser->triggerIndex);
    }
}

struct TimeSyncMasterToSlave {
    uint32_t currentTimeMs;
};

struct TimeSyncSlaveToMaster {
    int32_t variance;
};

void timeSyncMasterDataCallback(uint8_t address, uint8_t* data, uint8_t* dataSize) {
    TimeSyncMasterToSlave* response = (TimeSyncMasterToSlave*) data;
    response->currentTimeMs = millis();
    *dataSize = sizeof(TimeSyncMasterToSlave);
}

void timeSyncSlaveDataCallback(uint8_t* data, uint8_t dataSize, uint8_t* response, uint8_t* responseSize) {
    if(dataSize != sizeof(TimeSyncMasterToSlave)) {
        Serial.println("Invalid size");
        return;
    }
    TimeSyncMasterToSlave* masterToSlave = (TimeSyncMasterToSlave*) data;
    long timeSyncOffsetBefore = timeSyncOffset;
    lastTimeSyncMs = millis();
    timeSynced = true;
    timeSyncOffset = masterToSlave->currentTimeMs - millis();
    int32_t variance = timeSyncOffsetBefore - long(timeSyncOffset);
    *((TimeSyncSlaveToMaster*) response) = TimeSyncSlaveToMaster{ variance };
    *responseSize = sizeof(TimeSyncSlaveToMaster);
    Serial.printf("Synced time with master. offset: %i, variance: %i\n", timeSyncOffset, variance);
}

void timeSyncMasterReceiveCallback(uint8_t* data, uint8_t size, uint8_t slaveAddress) {
    if(size != sizeof(TimeSyncSlaveToMaster)) {
        Serial.println("Invalid size");
        return;
    }
    TimeSyncSlaveToMaster* slaveToMaster = (TimeSyncSlaveToMaster*) data;
    Serial.printf("Time sync with address %i. Variance: %ims\n", slaveAddress, slaveToMaster->variance);
}

void beginMasterSlaveLogic() {
    masterSlave.setMasterGotNewConnectionCallback(MasterGotNewConnectionCallbackFunc);
    masterSlave.setSlaveDisconnectedCallback(SlaveDisconnectedCallbackFunc);
    masterSlave.setSlaveLostMasterCallback(SlaveLostMasterCallbackFunc);
    masterSlave.setSlaveFoundMasterCallback(SlaveFoundMasterCallbackFunc);
    masterSlave.setMaster(isMasterCB->isChecked());
    masterSlave.addComunication(1, triggersMasterDataCallback, triggersSlaveDataCallback, triggersMasterReceiveCallback, TriggersMsgSlaveToMaster::getSize(3));
    masterSlave.addComunication(20, timeSyncMasterDataCallback, timeSyncSlaveDataCallback, timeSyncMasterReceiveCallback, sizeof(TimeSyncSlaveToMaster));
    masterSlave.begin();
}

void slaveTrigger(uint32_t atMs) {
    slaveTriggers.add(Trigger { atMs, getUid() });
    Serial.printf("Slave trigger(%i)\n", slaveTriggers.size());
}

void handleMasterSlaveLogic() {
    masterSlave.handle();
}