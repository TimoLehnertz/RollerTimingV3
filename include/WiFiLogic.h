/**
 * @file WiFiLogic.h
 * @author Timo Lehnertz
 * @brief File dealing with everything related to ESP32 WiFi
 * @version 0.1
 * @date 2023-07-11
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once
#include <Arduino.h>
#include <Global.h>
#include <WiFi.h>

uint16_t uid;

void beginWiFi() {
    WiFi.mode(WIFI_STA);
    uint8_t mac[6];
    WiFi.macAddress(mac);
    uid = *((uint16_t*)&mac[5]);
}

/**
 * @return uint16_t the last 2 bytes of the mac hoping that those uniquely identify each chip
 */
uint16_t getUid() {
    return uid;
}