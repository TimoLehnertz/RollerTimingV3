#include "MasterSlave.h"

void printFrame(Frame f, uint8_t size) {
    if(f.address == ADDRESS_BROADCAST) {
        Serial.print("Master -> broadcast: ");
    } else if(f.address == ADDRESS_MASTER) {
        Serial.print("Slave -> Master: ");
    } else {
        Serial.printf("Master -> #%i: ", f.address);
    }
    switch(f.frameType) {
        case FRAME_TYPE_MASTER_ARE_YOU_THERE:
            Serial.print("Are you there?");
            break;
        case FRAME_TYPE_MASTER_HEY_PLEASE_CONNECT_TO_ME:
            Serial.print("Connect to me");
            break;
        case FRAME_TYPE_SLAVE_I_WANT_TO_CONNECT:
            Serial.print("I want to connect");
            break;
        case FRAME_TYPE_SLAVE_YES_I_AM:
            Serial.print("Yes I am");
            break;
        default:
            Serial.printf("User defined comunication %i", f.frameType);
            break;
    }
    if(size > FRAME_HEADER_SIZE) {
        Serial.print(", data: ");
        for (size_t i = 0; i < size - FRAME_HEADER_SIZE; i++) {
            Serial.printf("%i,", f.data[i]);
        }
    }
    Serial.println();
}

MasterSlave::MasterSlave(bool master, TimeForSize timeForSizeCallback) {
    this->master = master;
    this->timeForSizeCallback = timeForSizeCallback;
    this->readFrame = Frame();
    this->writeFrame = Frame();
    this->readFrameSize = 0;
    this->writeFrameSize = 0;
    this->running = false;
    this->currentComunication = 0;
    this->connectionSize = 0;
    this->currentConnection = 0;
    this->nextMasterSendUs = 0;
    this->address = master ? 0 : -1;
    this->masterConnected = false;
    this->lastMasterFrame = 0;
    this->sendTimeUs = 0;
    this->currentComunicationReceived = false;
    this->comunications[0] = Comunication(10, nullptr, nullptr, nullptr, 3, FRAME_TYPE_MASTER_HEY_PLEASE_CONNECT_TO_ME, -1);
    this->comunications[1] = Comunication(10, nullptr, nullptr, nullptr, 3, FRAME_TYPE_MASTER_ARE_YOU_THERE, -1);
    this->comunicationsSize = 2;
    this->ConnectionRequestcomunication = &this->comunications[0];
    this->lastLQCalcMs = 0;
    this->slaveFoundMasterCallback = nullptr;
    this->slaveLostMasterCallback = nullptr;
    this->masterGotNewConnectionCallback = nullptr;
    this->slaveDisconnectedCallback = nullptr;
    this->comunicationDelay = 0;
}

void MasterSlave::setSlaveFoundMasterCallback(SlaveFoundMasterCallback slaveFoundMasterCallback) {
    this->slaveFoundMasterCallback = slaveFoundMasterCallback;
}

void MasterSlave::setSlaveLostMasterCallback(SlaveLostMasterCallback slaveLostMasterCallback) {
    this->slaveLostMasterCallback = slaveLostMasterCallback;
}

void MasterSlave::setMasterGotNewConnectionCallback(MasterGotNewConnectionCallback masterGotNewConnectionCallback) {
    this->masterGotNewConnectionCallback = masterGotNewConnectionCallback;
}

void MasterSlave::setSlaveDisconnectedCallback(SlaveDisconnectedCallback slaveDisconnectedCallback) {
    this->slaveDisconnectedCallback = slaveDisconnectedCallback;
}

bool MasterSlave::isMaster() {
    return master;
}

bool MasterSlave::isMasterConnected() {
    return masterConnected;
}

Connection* MasterSlave::getConnectionByAddress(uint8_t address) {
    for (size_t i = 0; i < connectionSize; i++) {
        if(connections[i].address == address) {
            return &connections[i];
        }
    }
    return nullptr;
}

Connection* MasterSlave::getConnectionByIndex(uint8_t index) {
    if(index >= connectionSize) return nullptr;
    return &connections[index];
}

bool MasterSlave::begin() {
    // if(comunicationsSize == 0) return false;
    currentComunication = 0;
    running = true;
    return true;
}

void MasterSlave::handle() {
    if(!running) return;
    if(millis() - lastLQCalcMs > 2000) {
        calculateLQ();
        lastLQCalcMs = millis();
    }
    if(master) {
        if(micros() >= nextMasterSendUs) {
            if(!currentComunicationReceived && currentConnection < connectionSize) connections[currentConnection].timeouts++;
            nextMasterComunication();
        }
        timeoutSlaves();
    } else {
        if(masterConnected && micros() - lastMasterFrame > MASTER_SLAVE_TIMEOUT_MS * 1000) {
            masterConnected = false;
            address = -1;
            if(slaveLostMasterCallback) slaveLostMasterCallback();
            return;
        }
    }
}

void MasterSlave::calculateLQ() {
    for (size_t i = 0; i < connectionSize; i++) {
        connections[i].lq = float(connections[i].receivedPackets) / float(connections[i].attemptedPackets) * 100;
        connections[i].receivedPackets = 0;
        connections[i].attemptedPackets = 0;
        Serial.printf("LQ for %i: %i%\n", connections[i].address, connections[i].lq);
    }
}

void MasterSlave::nextMasterComunication() {
    currentComunicationReceived = false;
    bool cycleFinished = false;
    for (size_t i = 0; i < (connectionSize + 1) * comunicationsSize; i++) { // maximum number of combinations to test
        // flow
        currentComunication++;
        if(currentComunication >= comunicationsSize) {
            currentComunication = 0;
            currentConnection++;
            if(currentConnection >= connectionSize) {
                currentConnection = 0;
            }
        }
        if(comunications[currentComunication].active == false) continue;
        if(comunications[currentComunication].isBroadcast()) { // broadcast
            if(currentConnection == 0) {
                if(comunications[currentComunication].priority <= comunications[currentComunication].skippedCount) {
                    sendMaserComunication(currentComunication, currentConnection);
                    comunications[currentComunication].skippedCount = 0;
                    return;
                } else {
                    comunications[currentComunication].skippedCount++;
                    continue;
                }
            } else {
                continue; // only send broadcasts when currentConnection is 0
            }
        } else if(currentConnection < connectionSize) { // no broadcast
            if(comunications[currentComunication].priority <= comunications[currentComunication].skippedCount) {
                sendMaserComunication(currentComunication, currentConnection);
                connections[currentConnection].attemptedPackets++;
                comunications[currentComunication].skippedCount = 0;
                return;
            } else {
                comunications[currentComunication].skippedCount++;
            }
        }
    }
}

void MasterSlave::timeoutSlaves() {
    for (size_t i = 0; i < connectionSize; i++) {
        if(millis() - connections[i].lastPacketMs > MASTER_SLAVE_TIMEOUT_MS) {
            Serial.printf("Slave with address %i timed out\n", connections[i].address);
            if(slaveDisconnectedCallback) slaveDisconnectedCallback(connections[i].address);
            for (size_t connectionIndex = i; i < connectionSize - 1; i++) {
                connections[i] = connections[i + 1];
            }
            connectionSize--;
        }
    }
}

uint8_t MasterSlave::getConnectedCount() {
    return connectionSize;
}

uint8_t MasterSlave::generateNewAddress() {
    uint8_t address = SLAVE_ADDRESS_OFFSET;
    for (address; address < 255; address++) {
        bool found = false;
        for (size_t i = 0; i < connectionSize; i++) {
            if(connections[i].address == address) {
                found = true;
                break;
            }
        }
        if(!found) return address;
    }
    return 255;
}

void MasterSlave::sendMaserComunication(uint8_t comunication, uint8_t connection) {
    writeFrame = Frame();
    writeFrame.address = connections[connection].address;
    writeFrame.frameType = comunications[comunication].frameType;
    switch(comunications[comunication].frameType) {
        case FRAME_TYPE_MASTER_HEY_PLEASE_CONNECT_TO_ME: {
            if(connectionSize >= MAX_CONNECTIONS) return;
            writeFrameSize = FRAME_HEADER_SIZE + 2;
            writeFrame.address = ADDRESS_BROADCAST;
            writeFrame.data[0] = generateNewAddress();
            writeFrame.data[1] = getConnectedCount();
            writeFrameSize = FRAME_HEADER_SIZE + 2;
            break;
        }
        case FRAME_TYPE_MASTER_ARE_YOU_THERE: {
            writeFrameSize = FRAME_HEADER_SIZE;
            break;
        }
        default: {// user defined comunication
            uint8_t size = 0;
            comunications[comunication].masterCallback(connections[connection].address, (uint8_t*) &writeFrame.data, &size);
            writeFrameSize = size + FRAME_HEADER_SIZE;
            break;
        }
    }
    nextMasterSendUs = micros() + calculateMaxTimeForFrameType(comunications[currentComunication].frameType) + comunicationDelay;
}

uint64_t MasterSlave::calculateMaxTimeForFrameType(uint8_t frameType) {
    if(frameType <= FRAME_TYPE_MAX) {
        return timeForSizeCallback(MAX_CONTROLL_PACKET_SIZE) * 2 + PAUSE_TILL_RESPONSE_US + TIMEOUT_US;
    } else {
        uint8_t comunicationsIndex = frameTypeToComunicationIndex(frameType);
        size_t size = comunications[comunicationsIndex].maxSlaveResponseSize + writeFrameSize;
        return timeForSizeCallback(size) + TIMEOUT_US;
    }
}

void MasterSlave::read(uint8_t* data, size_t size) {
    readFrame = *((Frame*) data);
    if(size < FRAME_HEADER_SIZE) {
        Serial.println("Received incomplete header frame");
        return;
    }
    if(master) {
        if(readFrame.address == ADDRESS_MASTER) {
            // Serial.print("<<");
            // printFrame(readFrame, size);
            processFrameAsMaster(readFrame, size);
        }
    } else {
        if(readFrame.address == address || readFrame.address == ADDRESS_BROADCAST) {
            // Serial.print("<<");
            // printFrame(readFrame, size);
            processFrameAsSlave(readFrame, size);
        }
    }
}

void MasterSlave::setMaster(bool master) {
    if(master == this->master) return;
    connectionSize = 0;
    masterConnected = false;
    address = -1;
    sendTimeUs = 0;
    if(master) {
        Serial.println("I am now master");
    } else {
        Serial.println("I am now slave");
    }
    this->master = master;
}

void MasterSlave::processFrameAsMaster(Frame& frame, size_t size) {
    if(frame.frameType == FRAME_TYPE_SLAVE_I_WANT_TO_CONNECT) { // try to establish connection
        if(generateNewAddress() != frame.data[0]) {
            Serial.println("Invalid accepted address");
            return;
        }
        addConnection(frame.data[0]);
        return;
    }
    if(frame.frameType > FRAME_TYPE_MAX && frame.frameType != comunications[currentComunication].frameType) { // check for errors
        Serial.printf("Invalid frame type %i. expected: %i\n", frame.frameType, comunications[currentComunication].frameType);
        return;
    }
    currentComunicationReceived = true;
    connections[currentConnection].lastPacketMs = millis();
    if(frame.frameType <= FRAME_TYPE_MAX) {
        switch(frame.frameType) {
            case FRAME_TYPE_SLAVE_YES_I_AM: {
                break; // do nothing only record received time
            }
            default: {
                Serial.printf("Invalid frame type %i\n", frame.frameType);
                break;
            }
        }
    } else { // user defined comunication
        uint8_t comunicationIndex = frameTypeToComunicationIndex(frame.frameType);
        if(comunicationIndex >= comunicationsSize) {
            Serial.printf("Invalid frame type %i\n", frame.frameType);
            return;
        }
        comunications[comunicationIndex].masterReceiveCallback(frame.data, size - FRAME_HEADER_SIZE, connections[currentConnection].address);
        comunications[comunicationIndex].maxComunications--;
        if(comunications[comunicationIndex].maxComunications == 0) {
            removeComunication(comunicationIndex);
        }
    }
    if(currentConnection < connectionSize) {
        connections[currentConnection].receivedPackets++;
    }
    nextMasterSendUs = micros() + PAUSE_TILL_RESPONSE_US + comunicationDelay;
}

void MasterSlave::removeComunication(uint8_t index) {
    for (size_t i = index; i < comunicationsSize - 1; i++) {
        comunications[i] = comunications[i + 1];
    }
    comunicationsSize--;
}

void MasterSlave::addConnection(uint8_t address) {
    Serial.printf("Added connection on address %i\n", address);
    connections[connectionSize++] = Connection(address, millis());
    if(masterGotNewConnectionCallback) masterGotNewConnectionCallback(&connections[connectionSize - 1]);
}

void MasterSlave::processFrameAsSlave(Frame& frame, size_t size) {
    if(masterConnected && frame.address == address) {
        lastMasterFrame = micros();
    }
    switch(frame.frameType) {
        case FRAME_TYPE_MASTER_ARE_YOU_THERE: {
            writeFrame.address = ADDRESS_MASTER;
            writeFrame.frameType = FRAME_TYPE_SLAVE_YES_I_AM;
            writeFrameSize = FRAME_HEADER_SIZE;
            break;
        }
        case FRAME_TYPE_MASTER_HEY_PLEASE_CONNECT_TO_ME: {
            if(masterConnected) return;
            address = frame.data[0];
            writeFrame.address = ADDRESS_MASTER;
            writeFrame.frameType = FRAME_TYPE_SLAVE_I_WANT_TO_CONNECT;
            writeFrame.data[0] = address;
            writeFrameSize = FRAME_HEADER_SIZE + 1;
            lastMasterFrame = micros();
            masterConnected = true; // assuming that connection was established succsessfully
            Serial.println("Connected to master");
            if(slaveFoundMasterCallback) slaveFoundMasterCallback(address);
            break;
        }
        default: {
            uint8_t comunicationIndex = frameTypeToComunicationIndex(frame.frameType);
            if(comunicationIndex >= comunicationsSize) {
                Serial.printf("Invalid master frame type: %i\n", frame.frameType);
                return;
            }
            uint8_t responseSize = 0;
            writeFrame.address = ADDRESS_MASTER;
            writeFrame.frameType = frame.frameType;
            comunications[comunicationIndex].slaveCallback(frame.data, size - FRAME_HEADER_SIZE, (uint8_t*) &writeFrame.data, &responseSize);
            writeFrameSize = responseSize + FRAME_HEADER_SIZE;
        }
    }
    sendTimeUs = micros() + PAUSE_TILL_RESPONSE_US;
}

size_t MasterSlave::getSize() {
    if(micros() < sendTimeUs) return 0;
    return writeFrameSize;
}

uint8_t* MasterSlave::getData() {
    // Serial.print(">>");
    // printFrame(writeFrame, writeFrameSize);
    writeFrameSize = 0; // clear current writeFrame
    return (uint8_t*)&writeFrame;
}

uint8_t MasterSlave::frameTypeToComunicationIndex(uint8_t frameType) {
    return frameType - FRAME_TYPE_MAX - 1;
}

uint8_t MasterSlave::comunicationIndexToFrameType(uint8_t comunicationIndex) {
    return comunicationIndex + FRAME_TYPE_MAX + 1;
}

uint8_t MasterSlave::addComunication(uint8_t priority, MasterCallback masterCallback, SlaveCallback slaveCallback, MasterReceiveCallback masterReceiveCallback, uint8_t maxSlaveResponseSize, int maxComunications) {
    if(comunicationsSize == MAX_COMUNICATION_SIZE) return 0;
    uint8_t frameType = comunicationIndexToFrameType(comunicationsSize);
    comunications[comunicationsSize] = Comunication(priority, masterCallback, slaveCallback, masterReceiveCallback, maxSlaveResponseSize, frameType, maxComunications);
    comunicationsSize++;
    return comunicationsSize - 1;
}