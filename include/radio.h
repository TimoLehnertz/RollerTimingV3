#pragma once

#include <Global.h>
#include <MasterSlaveLogic.h>

timeMs_t sendTimeout = 0;

void radioReceived(const uint8_t* byteArr, size_t size);

timeMs_t lastSend = 0;
timeMs_t receiveTimeout = 0;

struct SceduledSend {
    size_t size;
    uint8_t data[10];
};

DoubleLinkedList<SceduledSend> sceduledSends = DoubleLinkedList<SceduledSend>();

bool timeSyncRequested = false;

void sceduleSend(const uint8_t* data, size_t size) {
    if(size > 10) {
      Serial.println("Sceduled too large packet");
      return;
    }
    Serial.printf("secduling size %i\n", size);
    SceduledSend sceduledSend;
    memcpy(sceduledSend.data, data, size);
    sceduledSend.size = size;
    sceduledSends.pushBack(sceduledSend);
}

void sceduleTimeSync() {
  timeSyncRequested = true;
}

int64_t timeForSize(uint8_t size) {
  return radio.getTimeOnAir(size);
}

void beginRadio() {
  SPI.begin(LoRa_SCK, LoRa_MISO, LoRa_MOSI, LoRa_nss);

  Serial.print(F("[SX1262] Initializing ... "));
  // float freq = 434.0, float bw = 125.0, uint8_t sf = 9, uint8_t cr = 7, uint8_t syncWord = RADIOLIB_SX126X_SYNC_WORD_PRIVATE, int8_t power = 10
  // int error = radio.beginFSK(868);
  int error = radio.begin(868);
  // radio.setBandwidth(250);
  // radio.setPreambleLength(4);
  radio.setOutputPower(22); // 10 => 10mW, max: 22 => 158mW
  sendTimeout = radio.getTimeOnAir(sizeof(Trigger) * 2) / 1000;
  Serial.printf("Radio send timeout: %ims\n", sendTimeout);
  if (error == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(error);
    while (true);
  }
  radio.setDio1Action(setFlag);

  Serial.print(F("[SX1280] Starting to listen ... "));
  error = radio.startReceive();
  if (error == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(error);
    while (true);
  }
}

// this function is called when a complete packet is received or transmitted
ICACHE_RAM_ATTR void setFlag(void) {
  receivedFlag = true;
}

void handleRadioReceive() {
  if(receivedFlag) {
    receivedFlag = false;
    uint8_t byteArr[255];
    int error = radio.readData(byteArr, 255);
    size_t size = radio.getPacketLength();
    // Serial.printf("received %i bytes\n", size);
    if(size > 0) {
      if(error == RADIOLIB_ERR_NONE) {
        if(millis() > receiveTimeout) {
          Serial.printf("Received (len=%i)\n", size);
          radioReceived(byteArr, size);
        }
      } else if (error == RADIOLIB_ERR_CRC_MISMATCH) {
        Serial.println(F("CRC error!"));
      } else {
        // some other error occurred
        Serial.print(F("failed, code "));
        Serial.println(error);
      }
    }
    radio.startReceive();
  }
}

void handleRadioSend() {
  if(timeSyncRequested && millis() - lastSend > sendTimeout) {
    uint32_t time = millis();
    radio.startTransmit((uint8_t*) &time, sizeof(uint32_t));
    timeSyncRequested = false;
    receiveTimeout = millis() + radio.getTimeOnAir(sizeof(uint32_t)) / 1000 + 30;
    lastSend = millis();
    return;
  }
  if(sceduledSends.getSize() > 0 && millis() - lastSend > sendTimeout) {
      SceduledSend& sceduledSend = sceduledSends.getFirst();
      Serial.printf("Expected time on air: %ims, size: %i\n", radio.getTimeOnAir(sceduledSend.size) / 1000, sceduledSend.size);
      radio.startTransmit(sceduledSend.data, sceduledSend.size);
      receiveTimeout = millis() + radio.getTimeOnAir(sceduledSend.size) / 1000 + 30;
      sceduledSends.removeIndex(0);
      lastSend = millis();
      Serial.println("Sending");
  }
}