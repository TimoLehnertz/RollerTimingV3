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

struct SlaveLaser {
    Connection* connection; // actual MasterSlave connection containing metadata
    uint16_t triggerIndex; // holds the index of the last received trigger
};

LinkedList<SlaveLaser*> slaveLasers = LinkedList<SlaveLaser*>();

void SlaveFoundMasterCallback(uint8_t ownAddress) {
    Serial.printf("Found master. New address: %i\n", ownAddress);
}

void SlaveLostMasterCallback() {
    Serial.println("Master disconnected");
}

void MasterGotNewConnectionCallback(Connection* connection) {
    Serial.printf("New connection on address %i\n", connection->address);
    slaveLasers.add(new SlaveLaser{ connection, 0 });
}

void SlaveDisconnectedCallback(uint8_t slaveAddress) {
    Serial.printf("Slave %i disconnected\n", slaveAddress);
    for (size_t i = 0; i < slaveLasers.size(); i++) {
        SlaveLaser* slaveLaser = slaveLasers.get(i);
        if(slaveLaser->connection->address == slaveAddress) {
            slaveLasers.remove(i);
            delete slaveLaser;
            return;
        }
    }
    Serial.println("Error while removing slave laser");
}

SlaveLaser* getLaserByAddress(uint8_t address) {
    for (size_t i = 0; i < slaveLasers.size(); i++) {
        if(slaveLasers.get(i)->connection->address == address) {
            return slaveLasers.get(i);
        }
    }
    return nullptr;
}

struct MasterToSlave {
    union {
        struct {
            uint16_t triggerIndex; // the trigger index
        };
        uint8_t data[2];
    };
};

void masterDataCallback(uint8_t address, uint8_t* data, uint8_t* dataSize) {
    SlaveLaser* slaveLaser = getLaserByAddress(address);
    MasterToSlave masterToSlave = MasterToSlave{ slaveLaser->triggerIndex };
    memcpy(&masterToSlave, data, sizeof(MasterToSlave));
    *dataSize = sizeof(MasterToSlave);
}

void slaveDataCallback(uint8_t* data, uint8_t dataSize, uint8_t* response, uint8_t* responseSize) {
    if(dataSize != sizeof(MasterToSlave)) { // probably incorrect version
        *responseSize = 0;
        return;
    }
    MasterToSlave* masterToSlave = (MasterToSlave*) data;

}

void masterReceiveCallback(uint8_t* data, uint8_t size) {

}

void beginMasterSlaveLogic() {
    masterSlave.setMasterGotNewConnectionCallback(MasterGotNewConnectionCallback);
    masterSlave.setSlaveDisconnectedCallback(SlaveDisconnectedCallback);
    masterSlave.setSlaveLostMasterCallback(SlaveLostMasterCallback);
    masterSlave.setSlaveFoundMasterCallback(SlaveFoundMasterCallback);
    masterSlave.setMaster(isMasterCB->isChecked());
    masterSlave.addComunication(1, masterDataCallback, slaveDataCallback, masterReceiveCallback, 10);
    masterSlave.begin();
}

void handleMasterSlaveLogic() {
    masterSlave.handle();
}