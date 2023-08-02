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
#include <DoubleLinkedList.h>

#define MAX_SLAVE_TRIGGERS 256

void guiRemoveConnection(uint8_t address);
void guiSetConnection(uint8_t address, int millimeters, uint8_t lq, uint8_t stationType);

struct SlaveLaser {
    uint8_t address; // actual MasterSlave connection containing metadata
    uint16_t millimeters; // the number of millimeters between this station and the start
    uint16_t triggerIndex; // holds the index of the last received trigger
    uint16_t uid; // number identifying the device
    uint8_t stationType;
    bool connected;
};

DoubleLinkedList<SlaveLaser> slaveLasers = DoubleLinkedList<SlaveLaser>(); // for masters
DoubleLinkedList<Trigger> slaveTriggers = DoubleLinkedList<Trigger>(); // for slaves

bool timeSynced = false;
timeMs_t lastTimeSyncMs = 0;
timeMs_t timeSyncOffset = 0;

bool isTimeSynced() {
    return timeSynced;
}

timeMs_t localTimeToMasterTime(timeMs_t localTimeMs) {
    return localTimeMs + timeSyncOffset;
}

void addSlaveLaser(SlaveLaser slaveLaser) {
    Serial.printf("Discovered new laser on address %i. uid: %i\n", slaveLaser.address, slaveLaser.uid);
    slaveLasers.pushBack(slaveLaser);
}

SlaveLaser* getLaserByAddress(uint8_t address) {
    for (SlaveLaser& slaveLaser : slaveLasers) {
        if(slaveLaser.address == address) {
            return &slaveLaser;
        }
    }
    
    // for (size_t i = 0; i < slaveLasers.size(); i++) {
    //     if(slaveLasers.get(i)->address == address) {
    //         return slaveLasers.get(i);
    //     }
    // }
    return nullptr;
}

SlaveLaser* getLaserByUid(uint16_t uid) {
    for (SlaveLaser& slaveLaser : slaveLasers) {
        if(slaveLaser.uid == uid) {
            return &slaveLaser;
        }
    }
    // for (size_t i = 0; i < slaveLasers.size(); i++) {
    //     if(slaveLasers.get(i)->uid == uid) {
    //         return slaveLasers.get(i);
    //     }
    // }
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
    // gui will handle sound
}

void SlaveDisconnectedCallbackFunc(uint8_t slaveAddress) {
    Serial.printf("Slave with address %i disconnected\n", slaveAddress);
    SlaveLaser* slaveLaser = getLaserByAddress(slaveAddress);
    if(!slaveLaser) return;
    slaveLaser->connected = false;
    guiRemoveConnection(slaveAddress);
    // gui will handle sound
}

struct TriggersMsgMasterToSlave {
    uint16_t triggerIndex; // the trigger index
    uint16_t slaveUid;
};

struct TriggersMsgSlaveToMaster {

    uint16_t uid; // slave uid
    uint16_t millimeters; // slave uid
    uint8_t stationType;
    bool rebootedFlag;
    bool triggerFlag;
    Trigger trigger; // 0 - 3 triggers
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
    slaveToMaster.millimeters = distFromStartInput->getValue() * 1000;
    slaveToMaster.stationType = stationTypeSelect->getValue();
    slaveToMaster.triggerFlag = false;
    uint8_t sendTriggers = 0;
    // Serial.printf("Master wants %i, i got %i\n", masterToSlave->triggerIndex, slaveTriggers.size());
    if(masterToSlave->slaveUid == getUid()) {
        if(!isTimeSynced()) {
            // dont send any triggers yet
        } else if(masterToSlave->triggerIndex > slaveTriggers.getSize() || masterToSlave->triggerIndex >= MAX_SLAVE_TRIGGERS) { // Slave must have rebooted
            Serial.printf("I seem to have rebooted or have been overflowing. Master wants: %i, i got: %i\n", masterToSlave->triggerIndex, slaveTriggers.getSize());
            slaveTriggers.clear();
            slaveToMaster.rebootedFlag = true;
        } else {
            for (size_t i = masterToSlave->triggerIndex; i < min(int(slaveTriggers.getSize()), int(masterToSlave->triggerIndex + 1)); i++) { // send max 1 triggers
                Trigger& trigger = slaveTriggers.get(i);
                slaveToMaster.trigger = trigger;
                slaveToMaster.trigger.timeMs = localTimeToMasterTime(trigger.timeMs); // convert time
                slaveToMaster.triggerFlag = true;
                Serial.printf("Sending trigger of type: %i\n", trigger.triggerType);
            }
        }
    } else {
        Serial.printf("uid missmatch %i != %i\n", masterToSlave->slaveUid, getUid());
    }
    *responseSize = sizeof(TriggersMsgSlaveToMaster);
    memcpy(response, &slaveToMaster, *responseSize);
}

void triggersMasterReceiveCallback(uint8_t* data, uint8_t size, uint8_t slaveAddress) {
    // Serial.printf("slave data (%i) from %i\n", size, slaveAddress);
    if(size < sizeof(TriggersMsgSlaveToMaster)) return;
    TriggersMsgSlaveToMaster* slaveToMaster = (TriggersMsgSlaveToMaster*) data;
    SlaveLaser* slaveLaser = getLaserByUid(slaveToMaster->uid);
    if(!slaveLaser) {
        SlaveLaser slaveLaser = SlaveLaser{ slaveAddress, slaveToMaster->millimeters, 0, slaveToMaster->uid, slaveToMaster->stationType, true };
        addSlaveLaser(slaveLaser);
        return; // only add for now
    }
    slaveLaser->address = slaveAddress; // in case address changed
    slaveLaser->millimeters = slaveToMaster->millimeters;
    slaveLaser->stationType = slaveToMaster->stationType;
    if(slaveToMaster->rebootedFlag) {
        Serial.println("Slave has rebooted. Resetting triggerIndex");
        slaveLaser->triggerIndex = 0;
    } else if(slaveToMaster->triggerFlag) {
        slaveLaser->triggerIndex++;
        masterTrigger(slaveToMaster->trigger);
        Serial.printf("Received trigger for master time: %i from uid: %i at address %i, new triggerindex: %i, triggerType: %i\n", slaveToMaster->trigger.timeMs, slaveToMaster->uid, slaveAddress, slaveLaser->triggerIndex, slaveToMaster->trigger.triggerType);
    }
}

struct TimeSyncMasterToSlave {
    timeMs_t currentTimeMs;
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
    masterSlave.addComunication(1, triggersMasterDataCallback, triggersSlaveDataCallback, triggersMasterReceiveCallback, sizeof(TriggersMsgSlaveToMaster));
    masterSlave.addComunication(50, timeSyncMasterDataCallback, timeSyncSlaveDataCallback, timeSyncMasterReceiveCallback, sizeof(TimeSyncSlaveToMaster));
    masterSlave.begin();
}

void slaveTrigger(timeMs_t atMs, uint8_t triggerType) {
    slaveTriggers.pushBack(Trigger { atMs, uint16_t(distFromStartInput->getValue() * 1000), triggerType });
    Serial.printf("Slave trigger #%i, triggerType: %i\n", slaveTriggers.getSize(), triggerType);
}

void handleMasterSlaveLogic() {
    masterSlave.handle();
    for (size_t i = 0; i < masterSlave.getConnectedCount(); i++) {
        Connection* connection = masterSlave.getConnectionByIndex(i);
        if(!connection) continue;
        SlaveLaser* slaveLaser = getLaserByAddress(connection->address);
        int millimeters = -1;
        uint8_t stationType = STATION_TRIGGER_TYPE_NONE;
        if(slaveLaser) {
            millimeters = slaveLaser->millimeters;
            stationType = slaveLaser->stationType;
        }
        guiSetConnection(connection->address, millimeters, connection->lq, stationType);
    }
}