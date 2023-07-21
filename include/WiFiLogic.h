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
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <SPIFFS.h>
#include <JsonBuilder.h>
#include <SPIFFSLogic.h>

const char* wifiSsid = "Roller-Timing";
const char* wifiPassword = "Speedskate";

DNSServer dnsServer;
AsyncWebServer server(80);

bool wifiRunning = false;

void handleIndex() {

}

// ?
String processor(const String& var){
  return String();
}

void beginWiFi() {
    Serial.println("Starting WiFi");
    WiFi.softAP(wifiSsid, wifiPassword);
    dnsServer.start(53, "*", WiFi.softAPIP());
    IPAddress ip = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(ip);
    sprintf(wifiIPStr, "IP: %s", ip.toString());
    Serial.println("wifiStatusStr:");
    Serial.println(wifiIPStr);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/index.html", String(), false, processor);
    });
    server.on("/loadmore.json", HTTP_GET, [](AsyncWebServerRequest *request) {
        DoubleLinkedList<SessionMetadata>* metas = spiffsLogic.getSessionMetas();
        JsonBuilder builder = JsonBuilder();
        builder.startObject();
        builder.addKey("sessions");
        builder.startArray();
        for (SessionMetadata &&metadata : metas) {
            builder.startObject();
            builder.addKey("TriggerCount");
            builder.addValue(metadata.id);
            builder.endObject();
        }
        builder.endArray();
        
        builder.endObject();
    });
    server.begin();
    
}

void endWiFi() {
    Serial.println("Stopping WiFi");
    server.end();
    WiFi.enableAP(false);
    WiFi.softAPdisconnect();
}

uint16_t getUid() {
    return uidInput->getValue();
    // return uid;
}

void updateWiFiUI() {

}

void setWiFiActive(bool active) {
    if(active == wifiRunning) return;
    if(!active) {
        endWiFi();
    }
    if(active) {
        beginWiFi();
    }
    wifiRunning = active;
}

void handleWiFi() {
    if(!wifiRunning) return;
    // WiFiClient client = server.available();
}