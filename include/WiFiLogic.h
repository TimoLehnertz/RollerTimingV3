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

const byte DNS_PORT = 53;

// The access points IP address and net mask
// It uses the default Google DNS IP address 8.8.8.8 to capture all 
// Android dns requests
IPAddress apIP(8, 8, 8, 8);
IPAddress netMsk(255, 255, 255, 0);

bool wifiRunning = false;

void handleIndexPage(AsyncWebServerRequest* request);
void handleNotFound(AsyncWebServerRequest* request);
void handleSessionsJson(AsyncWebServerRequest* request);
void handleSession(AsyncWebServerRequest* request);
void handleCaptive(AsyncWebServerRequest* request);
void handleInPositionMp3(AsyncWebServerRequest* request);
void handleSetMp3(AsyncWebServerRequest* request);
void handleBeepMp3(AsyncWebServerRequest* request);
void handleStartNow(AsyncWebServerRequest* request);

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {
    server.on("/", HTTP_GET, handleIndexPage);
    server.on("/sessions.json", HTTP_GET, handleSessionsJson);
    server.on("/session", HTTP_GET, handleSession);
    server.on("/inPosition.mp3", HTTP_GET, handleInPositionMp3);
    server.on("/set.mp3", HTTP_GET, handleSetMp3);
    server.on("/beep.mp3", HTTP_GET, handleBeepMp3);
    server.on("/startNow", HTTP_GET, handleStartNow);
    server.onNotFound(handleNotFound);
  }

  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
    // request->addInterestingHeader("ANY");
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    handleCaptive(request);
  }
};

// ?
String processor(const String& var) {
  return String();
}

void handleIndexPage(AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/index.html", String(), false, processor);
}

void handleCaptive(AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/captive.html", String(), false, processor);
}

void handleSession(AsyncWebServerRequest* request) {
    if(!request->hasParam("name")) {
        request->send(404, "text/plain", "please provide the session name");
        return;
    }
    String name = request->getParam("name")->value();
    int id = name.toInt();
    if(!spiffsLogic.hasTraining(name)) {
        request->send(404, "text/plain", "Session not found");
        return;
    }
    TrainingsSession session = spiffsLogic.getTraining(name);
    JsonBuilder builder = JsonBuilder();
    builder.startArray();
    for (auto &&trigger : session) {
        builder.startObject();
        builder.addKey("triggerType");
        builder.addValue(trigger.triggerType);
        builder.addKey("timeMs");
        builder.addValue(trigger.timeMs);
        builder.addKey("millimeters");
        builder.addValue(trigger.millimeters);
        builder.endObject();
    }
    builder.endArray();
    request->send(200, "application/json", builder.getJson());
}

void handleInPositionMp3(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(SPIFFS, "/inPosition.mp3", "audio/mpeg");
    request->send(response);
}

void handleSetMp3(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(SPIFFS, "/set.mp3", "audio/mpeg");
    request->send(response);
}

void handleBeepMp3(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(SPIFFS, "/beep.mp3", "audio/mpeg");
    request->send(response);
}

void handleStartNow(AsyncWebServerRequest* request) {
    request->send(200, "text/html", "OK");
    masterTrigger(Trigger { timeMs_t(millis()), 0, STATION_TRIGGER_TYPE_START});
}

void handleSessionsJson(AsyncWebServerRequest* request) {
    const DoubleLinkedList<TrainingsMeta>& metas = spiffsLogic.getTrainingsMetas();
    JsonBuilder builder = JsonBuilder();
    builder.startObject();
    builder.addKey("sessions");
    builder.startArray();
    for (TrainingsMeta &metadata : metas) {
        builder.startObject();
        builder.addKey("fileSize");
        builder.addValue(int(metadata.fileSize));
        builder.addKey("fileName");
        builder.addValue(metadata.fileName);
        builder.addKey("triggerCount");
        builder.addValue(int(metadata.fileSize / sizeof(Trigger)));
        builder.endObject();
    }
    builder.endArray();
    builder.addKey("bytesUsed");
    builder.addValue(int(spiffsLogic.getBytesUsed()));
    builder.addKey("bytesTotal");
    builder.addValue(int(spiffsLogic.getBytesTotal()));
    builder.endObject();
    request->send(200, "application/json", builder.getJson());
}

void handleNotFound(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(SPIFFS, "/notFound.html", "text/html");
    request->send(response);
    // handleIndexPage(request);
    // Redirect to the start page on every connection
    // request->redirect("/");
    // request->send(302, "text/plain", "Redirecting to the start page...");
}

void beginWiFi() {
    Serial.println("Starting WiFi");
    WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP(wifiSsid, wifiPassword);
    IPAddress ip = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(ip);
    sprintf(wifiIPStr, "IP: %s", ip.toString());
    Serial.println("wifiStatusStr:");
    Serial.println(wifiIPStr);

    dnsServer.setErrorReplyCode(DNSReplyCode::NoError); 
    dnsServer.start(DNS_PORT, "*", apIP);

    server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);

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
    dnsServer.processNextRequest();
    // WiFiClient client = server.available();
}