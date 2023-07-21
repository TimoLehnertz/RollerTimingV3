#pragma once

#include <Global.h>

int64_t timeForSize(uint8_t size) {
  return radio.getTimeOnAir(size);
}

void beginRadio() {
  SPI.begin(LoRa_SCK, LoRa_MISO, LoRa_MOSI, LoRa_nss);

  Serial.print(F("[SX1262] Initializing ... "));
  // float freq = 434.0, float bw = 125.0, uint8_t sf = 9, uint8_t cr = 7, uint8_t syncWord = RADIOLIB_SX126X_SYNC_WORD_PRIVATE, int8_t power = 10
  int error = radio.beginFSK(868);
  radio.setOutputPower(22); // 10 => 10mW, max: 22 => 158mW
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
        masterSlave.read(byteArr, size);
      } else if (error == RADIOLIB_ERR_CRC_MISMATCH) {
        // Serial.println(F("CRC error!"));
      } else {
        // some other error occurred
        Serial.print(F("failed, code "));
        Serial.println(error);
      }
    }
    radio.startReceive();
  }
}

void handleradioSend() {
  size_t size = masterSlave.getSize();
  if(size > 0) {
    int error = radio.startTransmit(masterSlave.getData(), size);
    if(error == RADIOLIB_ERR_NONE) {

    } else if (error == RADIOLIB_ERR_PACKET_TOO_LONG) {
      // the supplied packet was longer than 256 bytes
      Serial.println(F("too long!"));
    } else if (error == RADIOLIB_ERR_TX_TIMEOUT) {
      Serial.println(F("timeout!"));
    } else {
      Serial.print(F("failed, code "));
      Serial.println(error);
    }
  }
}