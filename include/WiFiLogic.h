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
#include <HTTPClient.h>

const char* APSsid = "Roller-Timing";
const char* APPassword = "Speedskate";

DNSServer dnsServer;
AsyncWebServer server(80);

const byte DNS_PORT = 53;

// The access points IP address and net mask
// It uses the default Google DNS IP address 8.8.8.8 to capture all 
// Android dns requests
IPAddress apIP(8, 8, 8, 8);
IPAddress netMsk(255, 255, 255, 0);

bool wifiRunning = false;

timeMs_t startGunTime = INT32_MAX;

void handleIndexPage(AsyncWebServerRequest* request);
void handleNotFound(AsyncWebServerRequest* request);
void handleSessionsJson(AsyncWebServerRequest* request);
void handleSession(AsyncWebServerRequest* request);
void handleCaptive(AsyncWebServerRequest* request);
void handleInPositionMp3(AsyncWebServerRequest* request);
void handleSetMp3(AsyncWebServerRequest* request);
void handleBeepMp3(AsyncWebServerRequest* request);
void handleStartIn(AsyncWebServerRequest* request);
void handleSettings(AsyncWebServerRequest* request);

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {
    server.on("/", HTTP_GET, handleIndexPage);
    server.on("/sessions.json", HTTP_GET, handleSessionsJson);
    server.on("/session", HTTP_GET, handleSession);
    server.on("/inPosition.mp3", HTTP_GET, handleInPositionMp3);
    server.on("/set.mp3", HTTP_GET, handleSetMp3);
    server.on("/beep.mp3", HTTP_GET, handleBeepMp3);
    server.on("/startIn", HTTP_GET, handleStartIn);
    server.on("/settings", HTTP_GET, handleSettings);
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
    size_t maxTriggerCount = 100000;
    if(request->hasParam("limit")) {
        maxTriggerCount = request->getParam("limit")->value().toInt();
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
    size_t i = 0;
    for (auto &&trigger : session) {
        i++;
        if(session.getSize() > maxTriggerCount && (i < session.getSize() - maxTriggerCount)) {
            continue;
        }
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

void handleStartIn(AsyncWebServerRequest* request) {
    if(!request->hasParam("delayMs")) {
        request->send(400, "text/html", "No delayMs");
    }
    timeMs_t delayMs = request->getParam("delayMs")->value().toInt();
    startGunTime = millis() + delayMs;
    request->send(200, "text/html", "OK");
}

void handleSettings(AsyncWebServerRequest* request) {
    if(request->hasParam("displayBrightness")) {
        int displayBrightness = request->getParam("displayBrightness")->value().toInt();
        displayBrightnessInput->setValue(displayBrightness);
    }
    if(request->hasParam("displayTime")) {
        int displayTime = request->getParam("displayTime")->value().toFloat();
        displayTimeInput->setValue(displayTime);
    }
    if(request->hasParam("fontSize")) {
        fontSizeSelect->setValue(request->getParam("fontSize")->value().toInt());
    }
    if(request->hasParam("username")) {
        username = request->getParam("username")->value();
        cloudUploadEnabled->setChecked(true);
    }
    if(request->hasParam("wifiSSID")) {
        wifiSSID = request->getParam("wifiSSID")->value();
    }
    if(request->hasParam("wifiPassword")) {
        wifiPassword = request->getParam("wifiPassword")->value();
    }
    writePreferences();
    request->send(200, "text/html", "OK");
}

void handleSessionsJson(AsyncWebServerRequest* request) {
    const DoubleLinkedList<TrainingsMeta>& metas = spiffsLogic.getTrainingsMetas();
    JsonBuilder builder = JsonBuilder();
    builder.startObject();
    builder.addKey("displayTime");
    builder.addValue(int(displayTimeInput->getValue()));
    builder.addKey("displayBrightness");
    builder.addValue(float(displayBrightnessInput->getValue()));
    builder.addKey("fontSize");
    builder.addValue(int(fontSizeSelect->getValue()));
    builder.addKey("username");
    builder.addValue(username);
    builder.addKey("wifiSSID");
    builder.addValue(wifiSSID);
    builder.addKey("wifiPassword");
    builder.addValue(wifiPassword);
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
    WiFi.softAP(APSsid, APPassword);
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


bool cloudUploadRunning = false;
timeMs_t cloudUploadStart = 0;

char uploadPopupMessage[20];

bool handleCloudUpload() {
    if(wifiRunning) return true;
    if(WiFi.status() == WL_CONNECTED) {
        sprintf(uploadPopupMessage, "Cloud upload..");
        uiManager.popup(uploadPopupMessage);
        uiManager.handle(true);
        Serial.printf("Successfully connected to %s!\n", wifiSSID);
        HTTPClient http;

        String url = "https://www.roller-results.com/api/index.php?uploadResults=1";

        Serial.printf("Uploading %i sessions to: %s with user: %s\n", spiffsLogic.getTrainingsMetas().getSize() - 2, url.c_str(), username);

        Serial.printf("active training: %s\n", spiffsLogic.getActiveTraining().getFileName());

        http.begin(url);
        http.addHeader("Content-Type", "application/json");
        size_t i = 0;
        bool succsess = true;
        DoubleLinkedList<String> uploadedFiles = DoubleLinkedList<String>();
        for (auto &&trainingsMeta : spiffsLogic.getTrainingsMetas()) {
            if(!spiffsLogic.hasTraining(trainingsMeta.fileName)) continue;
            if(trainingsMeta.isRunning) continue;
            Serial.printf("Uploading %s\n", trainingsMeta.fileName);
            TrainingsSession trainingsSession = spiffsLogic.getTraining(trainingsMeta.fileName);
            JsonBuilder builder = JsonBuilder();
            builder.startObject();
            builder.addKey("user");
            builder.addValue(username);
            builder.addKey("triggers");
            builder.startArray();
            for (Trigger &trigger : trainingsSession) {
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
            builder.endObject();

            int httpResponseCode = http.POST(builder.getJson());
            i++;
            if (httpResponseCode == 200) {
                String response = http.getString();
                Serial.println("Response from roller results: " + response);
                uiManager.handle(true);
                sprintf(uploadPopupMessage, "Uploaded %i/%i", i, spiffsLogic.getTrainingsMetas().getSize());
                uploadedFiles.pushBack(trainingsMeta.fileName);
            } else {
                sprintf(uploadPopupMessage, "Upload error: %i", httpResponseCode);
                uiManager.handle(true);
                Serial.printf("Error during upload request. Error code: %i abording upload process\n", httpResponseCode);
                succsess = false;
                break;
            }
        }
        if(succsess) {
            for (auto &&fileName : uploadedFiles) {
                if(spiffsLogic.deleteSession(fileName)) {
                    Serial.printf("Deleted %s\n", fileName);
                } else {
                    Serial.printf("Could not delete session %s\n", fileName);
                }
            }
            sprintf(uploadPopupMessage, "Upload done");
        }
        http.end();
        return true;
    } else if(millis() - cloudUploadStart > 5000) {
        Serial.printf("Could not connect to %s!\n", wifiSSID);
        return true;
    }
    return false;
}

void tryInitUpload() {
    Serial.printf("Connecting to %s...\n", wifiSSID);
    WiFi.begin(wifiSSID, wifiPassword);
    cloudUploadStart = millis();
    cloudUploadRunning = true;
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
    // start gun
    if(millis() > startGunTime) {
        masterTrigger(Trigger { startGunTime, 0, STATION_TRIGGER_TYPE_START});
        startGunTime = INT32_MAX;
    }
    static bool cloudUploadAttempted = false;
    if(!cloudUploadAttempted && cloudUploadEnabled->isChecked() && !wifiRunning && spiffsLogic.getTrainingsMetas().getSize() > 1) { // dont upload current session
        if(millis() > 5000) {
            tryInitUpload();
            cloudUploadAttempted = true;
        }
    }
    if(cloudUploadRunning) {
        cloudUploadRunning = !handleCloudUpload();
    }
    if(!wifiRunning) return;
    dnsServer.processNextRequest();
    // WiFiClient client = server.available();
}