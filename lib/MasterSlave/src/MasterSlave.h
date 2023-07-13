#pragma once
#include <Arduino.h>

#define MAX_PACKET_SIZE 255
#define FRAME_HEADER_SIZE 2

#define MAX_CONTROLL_PACKET_SIZE 4

#define TIMEOUT_US 100000 // maximum waiting time for responses

#define MASTER_SLAVE_TIMEOUT_MS 3000
#define PAUSE_TILL_RESPONSE_US 10000

#define MAX_COMUNICATION_SIZE 100

#define ADDRESS_MASTER 1
#define ADDRESS_BROADCAST 0
#define SLAVE_ADDRESS_OFFSET 2

// [0] provided address
// [1] already connected slaves
#define FRAME_TYPE_MASTER_HEY_PLEASE_CONNECT_TO_ME 1

// [0] accepted address
#define FRAME_TYPE_SLAVE_I_WANT_TO_CONNECT 2

#define FRAME_TYPE_MASTER_ARE_YOU_THERE 4
#define FRAME_TYPE_SLAVE_YES_I_AM 5

#define FRAME_TYPE_MAX FRAME_TYPE_SLAVE_YES_I_AM

#define MAX_CONNECTIONS 50

struct Frame {
    uint8_t address;
    uint8_t frameType;
    uint8_t data[MAX_PACKET_SIZE - FRAME_HEADER_SIZE];
};

/**
 * @param address the address of the recipient
 * @param data the data array to be filled. Maximum size: MAX_PACKET_SIZE - PACKET_HEADER_SIZE
 * @param dataSize the resulting size of data
 */
typedef void(*MasterCallback)(uint8_t address, uint8_t* data, uint8_t* dataSize);
/**
 * @param data pointer to data send by master
 * @param size of data
 * @param response to be filled. Maximum size: MAX_PACKET_SIZE - PACKET_HEADER_SIZE
 * @param responseSize size of response
 */
typedef void(*SlaveCallback)(uint8_t* data, uint8_t dataSize, uint8_t* response, uint8_t* responseSize);
/**
 * @param data pointer to data send by slave
 * @param size of data
 */
typedef void(*MasterReceiveCallback)(uint8_t* data, uint8_t size, uint8_t slaveAddress);

typedef uint32_t(*TimeForSize)(uint8_t size);

typedef void (*SlaveFoundMasterCallback)(uint8_t ownAddress);
typedef void (*SlaveLostMasterCallback)();

struct Connection;
typedef void (*MasterGotNewConnectionCallback)(Connection* connection);
typedef void (*SlaveDisconnectedCallback)(uint8_t slaveAddress);

struct Comunication {
    Comunication() {}
    Comunication(uint8_t priority, MasterCallback masterCallback, SlaveCallback slaveCallback, MasterReceiveCallback masterReceiveCallback, uint8_t maxSlaveResponseSize, uint8_t frameType, int maxComunications) : priority(priority), masterCallback(masterCallback), slaveCallback(slaveCallback), masterReceiveCallback(masterReceiveCallback), maxSlaveResponseSize(maxSlaveResponseSize), skippedCount(0), frameType(frameType), maxComunications(maxComunications), active(true) {}
    uint8_t priority;
    MasterCallback masterCallback;
    SlaveCallback slaveCallback;
    MasterReceiveCallback masterReceiveCallback;
    uint8_t maxSlaveResponseSize;
    uint8_t skippedCount;
    uint8_t frameType;
    int16_t maxComunications; // will get deleted when reached
    bool active;

    bool isBroadcast() {
        return frameType == FRAME_TYPE_MASTER_HEY_PLEASE_CONNECT_TO_ME;
    }
};

struct Connection {
    Connection() {}
    Connection(uint8_t address, uint32_t lastPacketMs) : address(address), lastPacketMs(lastPacketMs), timeouts(0), receivedPackets(0), attemptedPackets(0), lq(0) {}
    uint8_t address;
    uint32_t lastPacketMs;
    uint8_t timeouts;
    uint32_t receivedPackets;
    uint32_t attemptedPackets;
    uint8_t lq; // percentage from 0 to 100. 0 => 0 packets received, 100 => all packets received
};

class MasterSlave {
public:
    MasterSlave(bool master, TimeForSize timeForSizeCallback);
    
    bool begin();
    void handle();
    void read(uint8_t* data, size_t size);

    size_t getSize();
    uint8_t* getData();

    uint8_t addComunication(uint8_t priority, MasterCallback masterCallback, SlaveCallback slaveCallback, MasterReceiveCallback masterReceiveCallback, uint8_t maxSlaveResponseSize, int maxComunications = -1);

    void setMaster(bool master);

    uint8_t getConnectedCount();

    void setSlaveFoundMasterCallback(SlaveFoundMasterCallback slaveFoundMasterCallback);
    void setSlaveLostMasterCallback(SlaveLostMasterCallback slaveLostMasterCallback);
    void setMasterGotNewConnectionCallback(MasterGotNewConnectionCallback masterGotNewConnectionCallback);
    void setSlaveDisconnectedCallback(SlaveDisconnectedCallback slaveDisconnectedCallback);

    bool isMaster();

    bool isMasterConnected();

    Connection* getConnectionByAddress(uint8_t address);
    Connection* getConnectionByIndex(uint8_t index);

private:
    bool master;
    TimeForSize timeForSizeCallback;
    Frame writeFrame;
    size_t writeFrameSize;
    Frame readFrame;
    size_t readFrameSize;
    uint64_t nextMasterSendUs;

    Comunication comunications[MAX_COMUNICATION_SIZE];
    size_t comunicationsSize;

    bool running;

    uint8_t currentComunication;

    Connection connections[MAX_CONNECTIONS]; // can have gaps of nullptrs
    uint8_t connectionSize;
    uint8_t currentConnection;
    int address;

    uint64_t lastMasterFrame;
    bool masterConnected; // only for slaves

    bool currentComunicationReceived;

    uint32_t lastLQCalcMs;

    // time to send next packet
    uint64_t sendTimeUs;

    Comunication* ConnectionRequestcomunication;

    SlaveFoundMasterCallback slaveFoundMasterCallback;
    SlaveLostMasterCallback slaveLostMasterCallback;
    MasterGotNewConnectionCallback masterGotNewConnectionCallback;
    SlaveDisconnectedCallback slaveDisconnectedCallback;

    void processFrameAsMaster(Frame& frame, size_t size);
    void processFrameAsSlave(Frame& frame, size_t size);

    void nextMasterComunication();

    void sendMaserComunication(uint8_t comunication, uint8_t connection);

    void addConnection(uint8_t address);

    uint8_t generateNewAddress();

    uint8_t frameTypeToComunicationIndex(uint8_t frameType);
    uint8_t comunicationIndexToFrameType(uint8_t comunicationIndex);

    void removeComunication(uint8_t index);

    uint64_t calculateMaxTimeForFrameType(uint8_t frameType);

    void timeoutSlaves();

    void calculateLQ();
};