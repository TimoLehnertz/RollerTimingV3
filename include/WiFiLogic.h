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
// #include <DNSServer.h>
#include <SPIFFS.h>
#include <JsonBuilder.h>
#include <SPIFFSLogic.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Storage.h>
#include <GuiLogic.h>

#define APSSID_DISPLAY_DEFAULT "RollerTiming"
#define APSSID_STATION_DEFAULT "RollerTiming Station"
#define APPASSWORD_DEFAULT "GettingFaster"

String APSsid = APSSID_DISPLAY_DEFAULT;
String APPassword = APPASSWORD_DEFAULT;

// DNSServer dnsServer;
AsyncWebServer server(80);

AsyncEventSource liveEventHandler("/live");

// const byte DNS_PORT = 53;

// The access points IP address and net mask
// It uses the default Google DNS IP address 8.8.8.8 to capture all
// Android dns requests
IPAddress apIP(8, 8, 8, 8);
IPAddress netMsk(255, 255, 255, 0);

bool shouldReboot = false;

timeMs_t startGunTime = INT32_MAX;

timeMs_t restardWifiTime = 0; // can be set to a time to restard wifi at that time. Used to update credentials

void handleIndexPage(AsyncWebServerRequest *request);
void handleNotFound(AsyncWebServerRequest *request);
void handleSessionsJson(AsyncWebServerRequest *request);
void handleSession(AsyncWebServerRequest *request);
void handleCaptive(AsyncWebServerRequest *request);
void handleInPositionMp3(AsyncWebServerRequest *request);
void handleSetMp3(AsyncWebServerRequest *request);
void handleBeepMp3(AsyncWebServerRequest *request);
void handleStartIn(AsyncWebServerRequest *request);
void handleSettings(AsyncWebServerRequest *request);
void handleDeleteSession(AsyncWebServerRequest *request);
void handleLogo(AsyncWebServerRequest *request);
void handleUserManual(AsyncWebServerRequest *request);
void handleWiFiSettings(AsyncWebServerRequest *request);
void handleUpdatePage(AsyncWebServerRequest *request);
DoubleLinkedList<SessionPageInfo> buildSessionTriggers(JsonBuilder &builder, TrainingsSession &session, size_t page);

void beginWiFi();
// void endWiFi();
void writePreferences();
void wiFiCredentialsChanged();

// ?
String processor(const String &var)
{
    return String();
}

void handleIndexPage(AsyncWebServerRequest *request)
{
    if (isDisplaySelect->getValue())
    { // is display
        request->send(SPIFFS, "/index.html", String("text/html"), false, processor);
    }
    else
    {
        handleUpdatePage(request);
    }
}

void handleCaptive(AsyncWebServerRequest *request)
{
    request->send(SPIFFS, "/captive.html", String("text/html"), false, processor);
}

void resolveLiveRequests(size_t newTriggerCount)
{
    Serial.println("Resolving live requests");
    TrainingsSession cachedSession = spiffsLogic.getActiveTraining();
    JsonBuilder builder = JsonBuilder();
    builder.startArray();
    builder.insertTriggerObj(cachedSession.getCache().getLast()); // only last trigger
    // for (auto &&trigger : cachedSession.getCache()) {
    //     builder.insertTriggerObj(trigger);
    // }
    builder.endArray();
    String json = builder.getJson();
    liveEventHandler.send(json.c_str(), "update", millis());
}

void handleLiveConnect(AsyncEventSourceClient *client)
{
    if (client->lastId())
    {
        Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second

    TrainingsSession cachedSession = spiffsLogic.getActiveTraining();
    JsonBuilder builder = JsonBuilder();
    builder.startArray();
    for (auto &&trigger : cachedSession.getCache())
    {
        builder.insertTriggerObj(trigger);
    }
    builder.endArray();
    client->send(builder.getJson().c_str(), "init", millis());
}

void handleSession(AsyncWebServerRequest *request)
{
    if (!request->hasParam("name"))
    {
        request->send(404, "text/plain", "please provide the session name");
        return;
    }
    String name = request->getParam("name")->value();
    int id = name.toInt();
    if (!spiffsLogic.hasTraining(name))
    {
        request->send(404, "text/plain", "Session not found");
        return;
    }
    TrainingsSession session = spiffsLogic.getTraining(name);
    size_t page = 0;
    if (request->hasParam("page"))
    {
        page = request->getParam("page")->value().toInt();
    }
    if (page >= session.getPageCount())
    {
        request->send(400, "text/plain", "Page doesnt exist");
        return;
    }
    JsonBuilder jsonBuilder;
    jsonBuilder.startObject();
    jsonBuilder.addKey("triggers");
    DoubleLinkedList<SessionPageInfo> pages = buildSessionTriggers(jsonBuilder, session, page);
    jsonBuilder.addKey("maxPages");
    jsonBuilder.addValue(int(pages.getSize()));
    jsonBuilder.endObject();

    request->send(200, "application/json", jsonBuilder.getJson());
}

void handleInPositionMp3(AsyncWebServerRequest *request)
{
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/inPosition.mp3", "audio/mpeg");
    request->send(response);
}

void handleSetMp3(AsyncWebServerRequest *request)
{
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/set.mp3", "audio/mpeg");
    request->send(response);
}

void handleBeepMp3(AsyncWebServerRequest *request)
{
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/beep.mp3", "audio/mpeg");
    request->send(response);
}

void handleLogo(AsyncWebServerRequest *request)
{
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/logo.png", "image/png");
    request->send(response);
}

void handleUserManual(AsyncWebServerRequest *request)
{
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/user-manual.html", "text/html");
    request->send(response);
}

void handleStartIn(AsyncWebServerRequest *request)
{
    if (!request->hasParam("delayMs"))
    {
        request->send(400, "text/html", "No delayMs");
    }
    timeMs_t delayMs = request->getParam("delayMs")->value().toInt();
    startGunTime = millis() + delayMs;
    request->send(200, "text/html", "OK");
}

void handleDeleteSession(AsyncWebServerRequest *request)
{
    if (!request->hasParam("name"))
    {
        request->send(400, "text/html", "Name missing");
    }
    if (spiffsLogic.getActiveTraining().getFileName().equals(request->getParam("name")->value()))
    {
        request->send(400, "text/html", "Cant delete active session");
    }
    if (!spiffsLogic.deleteSession(request->getParam("name")->value()))
    {
        request->send(500, "text/html", "Error");
    }
    else
    {
        request->send(200, "text/html", "Done");
    }
}

void handleUpdatePage(AsyncWebServerRequest *request)
{
    request->send(200, "text/html", (R""""(<!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <link rel="icon" type="image/png" href="/logo.png">
            <title>Roller timing</title>
        </head>
        <style>
        * {
            font-family: "Arial", sans-serif;
            color: #333;
        }
        body {
            margin: 0;
            padding: 1rem;
            background: linear-gradient(to bottom right, #4287f5, #b243f5);
            display: flex;
            justify-content: center;
            align-items: start;
            padding-top: 5rem;
            min-height: calc(100vh - 5rem);
        }
        .container {
            display: flex;
            justify-content: center;
            align-items: start;
            background-color: white;
            width: 40rem;
            border-radius: 20px;
            box-shadow: 0 2px 10px rgba(0, 0, 0, 0.2);
            padding: 30px;
        }
        h1 {
            font-size: 32px;
            margin-bottom: 20px;
        }
        p {
            font-size: 16px;
            color: #555;
            margin: 0;
        }
        .download-btn {
            border: none;
            display: inline-block;
            padding: 10px 10px;
            background-color: #4287f5;
            color: white;
            font-size: 15px;
            border-radius: 8px;
            text-decoration: none;
        }
        </style>
        <body>
            <div class="container">
                <div class="content">
                    <div class="error" id="error-message"></div>
                    <h1>Roller timing version 3.1</h1>
                    <br>
                    <p><b>Attention</b> Updating the firmware will erase <b>all</b> your settings and saved Trainings sessions. Only proceed if that is okay!</p>
                    <p>Follow all of the steps below closely in the correct order</p>
                    <br>
                    <br>
                    <p>While updating monitor the little blue display!</p>
                    <br>
                    <br><p>Upload procedure for displays:</p>
                    <ol>
                    <li>Choose "firmware.bin" and hit Update</li>
                    <li>The device will reboot and WiFi will disconnect. Thats normal. The update will take up to one minute. Don't cut power during the update!</li>
                    <li>Connect and navigate to <b>http://8.8.8.8</b> All pages except the update page will be disabled</li>
                    <li>Choose "spiffs.bin" and hit Update</li>
                    <li>The device will reboot and WiFi will disconnect. Thats normal. The update will take up to one minute. Don't cut power during the update!</li>
                    <li>Done</li>
                    </ol>
                    <hr>
                    <br><p>Upload procedure for stations:</p>
                    <ol>
                    <li>Choose "firmware.bin" and hit Update</li>
                    <li>The device will reboot and WiFi will disconnect. Thats normal. The update will take up to one minute. Don't cut power during the update!</li>
                    <li>Done</li>
                    </ol>
                    <form method='POST' action='/uploadUpdate' enctype='multipart/form-data' id='upload-form'>
                        <input type='file' name='update' accept=".bin" required>
                        <input type='submit' value='Update' class='download-btn'>
                    </form>
                    <br><hr><br>
                    <p>
                        Roller timing<br>
                        Software version: <span style="color: #24240f">3.1</span><br>
                        Roller results - From skaters for skaters<br>
                        by Timo Lehnertz
                    </p>
                </div>
            </div>
        </body>
        </html>)""""));
}

void handleWiFiSettings(AsyncWebServerRequest *request)
{
    // int params = request->params();
    // for (int i = 0; i < params; i++) {
    //     AsyncWebParameter* p = request->getParam(i);
    //     Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
    // }
    if (!request->hasParam("APSsid") && !request->hasParam("APPassword"))
    {
        Serial.println(request->hasParam("APSsid"));
        Serial.println(request->hasParam("APPassword"));
        request->send(400, "text/html", "bad request");
        return;
    }
    if (APSsid == request->getParam("APSsid")->value() && APPassword == request->getParam("APPassword")->value())
    { // nothing changed
        handleIndexPage(request);
        return;
    }
    APSsid = request->getParam("APSsid")->value();
    APPassword = request->getParam("APPassword")->value();
    wiFiCredentialsChanged();
    restardWifiTime = millis() + 500;
    writePreferences();
    request->redirect("/");
    // request->send(200, "text/html", String("Please reconnect to ") + String(APSsid));
}

void handleSettings(AsyncWebServerRequest *request)
{
    if (request->hasParam("displayBrightness"))
    {
        int displayBrightness = request->getParam("displayBrightness")->value().toInt();
        displayBrightnessInput->setValue(displayBrightness);
    }
    if (request->hasParam("displayTime"))
    {
        int displayTime = request->getParam("displayTime")->value().toFloat();
        displayTimeInput->setValue(displayTime);
    }
    if (request->hasParam("fontSize"))
    {
        fontSizeSelect->setValue(request->getParam("fontSize")->value().toInt());
    }
    if (request->hasParam("username"))
    {
        username = request->getParam("username")->value();
        cloudUploadEnabled->setChecked(true);
    }
    if (request->hasParam("wifiSSID"))
    {
        uploadWifiSSID = request->getParam("wifiSSID")->value();
    }
    if (request->hasParam("wifiPassword"))
    {
        uploadWifiPassword = request->getParam("wifiPassword")->value();
    }
    if (request->hasParam("lapDisplayType"))
    {
        const int lapDisplayType = request->getParam("lapDisplayType")->value().toInt();
        if (lapDisplayType >= 0 && lapDisplayType <= 1)
        {
            lapDisplayTypeSelect->setValue(lapDisplayType);
        }
    }
    writePreferences();
    request->send(200, "text/html", "OK");
}

void handleSessionsJson(AsyncWebServerRequest *request)
{
    const DoubleLinkedList<TrainingsMeta> &metas = spiffsLogic.getTrainingsMetas();
    JsonBuilder builder = JsonBuilder();
    builder.startObject();
    builder.addKey("displayTime");
    builder.addValue(int(displayTimeInput->getValue()));
    builder.addKey("displayBrightness");
    builder.addValue(float(displayBrightnessInput->getValue()));
    builder.addKey("fontSize");
    builder.addValue(int(fontSizeSelect->getValue()));
    builder.addKey("lapDisplayType");
    builder.addValue(lapDisplayTypeSelect->getValue());
    builder.addKey("username");
    builder.addValue(username);
    builder.addKey("wifiSSID");
    builder.addValue(uploadWifiSSID);
    builder.addKey("wifiPassword");
    builder.addValue(uploadWifiPassword);
    builder.addKey("APSsid");
    builder.addValue(APSsid);
    builder.addKey("APPassword");
    builder.addValue(APPassword);
    builder.addKey("sessions");
    builder.startArray();
    for (TrainingsMeta &metadata : metas)
    {
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

void handleNotFound(AsyncWebServerRequest *request)
{
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/notFound.html", "text/html");
    request->send(response);
    // handleIndexPage(request);
    // Redirect to the start page on every connection
    // request->redirect("/");
    // request->send(302, "text/plain", "Redirecting to the start page...");
}

/**
 * @brief Get the Triggers of a Session
 *
 * @param session the session
 * @param page = 0 will return only the newest Triggers
 * @param json output JSON
 */
DoubleLinkedList<SessionPageInfo> buildSessionTriggers(JsonBuilder &builder, TrainingsSession &session, size_t page)
{
    DoubleLinkedList<SessionPageInfo> sessionPageInfos = session.getSessionPages();
    SessionPageInfo sessionPage = sessionPageInfos.get(page);
    Serial.printf("Page %i from %i to %i\n", page, sessionPage.pageStart, sessionPage.pageEnd);
    session.beginStream();
    session.skip(sessionPage.pageStart);
    size_t currentTriggerIndex = sessionPage.pageStart;
    builder.startArray();
    while (session.hasNext() && currentTriggerIndex <= sessionPage.pageEnd)
    {
        builder.insertTriggerObj(session.next());
        currentTriggerIndex++;
    }
    builder.endArray();
    session.endStream();
    return sessionPageInfos;
}

bool updateSuccsessfull = false;

void beginWiFi()
{
    Serial.println("Starting WiFi");
    // WiFi.eraseAP();
    // WiFi.disconnect(false, true);
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP(APSsid, APPassword);
    IPAddress ip = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(ip);

    // dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    // dnsServer.start(DNS_PORT, "*", apIP);

    // server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
    if (isDisplaySelect->getValue() && spiffsLogic.isVersionMatch())
    { // is display and spiffs version is correct
        server.on("/", HTTP_GET, handleIndexPage);
        server.on("/sessions.json", HTTP_GET, handleSessionsJson);
        server.on("/session", HTTP_GET, handleSession);
        server.on("/inPosition.mp3", HTTP_GET, handleInPositionMp3);
        server.on("/set.mp3", HTTP_GET, handleSetMp3);
        server.on("/beep.mp3", HTTP_GET, handleBeepMp3);
        server.on("/startIn", HTTP_GET, handleStartIn);
        server.on("/settings", HTTP_GET, handleSettings);
        server.on("/deleteSession", HTTP_GET, handleDeleteSession);
        server.on("/logo.png", HTTP_GET, handleLogo);
        server.on("/updateCredentials", HTTP_GET, handleWiFiSettings);
        server.on("/user-manual.html", HTTP_GET, handleUserManual);
        server.on("/update", HTTP_GET, handleUpdatePage); // ipdate page has own url
        server.onNotFound(handleNotFound);
        server.addHandler(&liveEventHandler);
        liveEventHandler.onConnect(handleLiveConnect);
    }
    else
    {
        // everything is getting to the update page
        server.on("/", HTTP_GET, handleUpdatePage);
        server.onNotFound(handleUpdatePage);
    }
    // attach filesystem root at URL /fs
    // server.serveStatic("/fs", SPIFFS, "/");
    server.on(PSTR("/uploadUpdate"), HTTP_POST, [](AsyncWebServerRequest *request)
              {
                  shouldReboot = true;
                  // AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", updateSuccsessfull ? "Update succsessful. Rebooting... Rebooting will take up to a minute. Dont cut power!" : "Update failed");
                  // response->addHeader("Connection", "close");
                  // request->send(response);
              },
              [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
              {
        updateSuccsessfull = false;
        uiManager.popup("Updating...");
        uiManager.handle(true);
        if(!index) {
            if(!filename.startsWith("firmware") && !filename.startsWith("spiffs") && !filename.startsWith("1_firmware") && !filename.startsWith("2_spiffs")) {
                Serial.printf("invalid update file: %s\n", filename);
                uiManager.popup("Invalid file!");
                uiManager.handle(true);
                delay(3000);
                uiManager.popup("Upload valid File!");
                uiManager.handle(true);
                delay(3000);
                return;
            }
            if((filename.startsWith("spiffs") || filename.startsWith("2_spiffs")) && !isDisplaySelect->getValue()) {
                uiManager.popup("Wrong file!");
                uiManager.handle(true);
                delay(3000);
                uiManager.popup("Update firmware!");
                uiManager.handle(true);
                delay(3000);
                return; // stations dont need spiffs updates
            }
            Serial.printf("Update Start: %s\n", filename.c_str());
            if(filename.startsWith("firmware") || filename.startsWith("1_firmware")) {
                Serial.println("Updating firmware");
                uiManager.popup("Updating firmware");
                uiManager.handle(true);
                if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
                    Update.printError(Serial);
                }
            } else if(filename.startsWith("spiffs") || filename.startsWith("2_spiffs")) {
                Serial.println("Updating spiffs");
                uiManager.popup("Updating spiffs");
                uiManager.handle(true);
                if(!Update.begin(4294967295U, U_SPIFFS)) {
                //   if(!Update.begin(SPIFFS.totalBytes(), U_SPIFFS)) {
                    Update.printError(Serial);
                }
            }
        }
        if(!Update.hasError()) {
            if(Update.write(data, len) != len) {
                Update.printError(Serial);
            }
        }
        if(final) {
            if(Update.end(true)) {
                Serial.printf("Update Success: %uB\n", index+len);
                updateSuccsessfull = true;
                uiManager.popup("Succsess. Rebooting..");
                uiManager.handle(true);
                delay(1000);
            } else {
                Update.printError(Serial);
                uiManager.popup("Error. Rebooting..");
                uiManager.handle(true);
                delay(1000);
            }
        } });
    server.begin();
}

// void endWiFi() {
// Serial.println("Stopping WiFi");
// server.end();
// // WiFi.enableAP(false);
// // WiFi.softAPdisconnect();
// WiFi.disconnect();
// WiFi.mode(WIFI_OFF);
// wifiRunning = false;
// }

bool cloudUploadRunning = false;
timeMs_t cloudUploadStart = 0;

char uploadPopupMessage[20];

bool handleCloudUpload()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        FastLED.clear();
        FastLED.setBrightness(10);
        matrix.print("Uploading", 1, 1, CRGB::BlueViolet, FONT_SIZE_SMALL + FONT_SETTINGS_DEFAULT);
        FastLED.show();
        sprintf(uploadPopupMessage, "Cloud upload..");
        uiManager.popup(uploadPopupMessage);
        uiManager.handle(true);
        Serial.printf("Successfully connected to %s!\n", uploadWifiSSID);
        HTTPClient http;

        String url = "https://www.roller-results.com/api/index.php?uploadResults=1";

        Serial.printf("Uploading %i sessions to: %s with user: %s\n", spiffsLogic.getTrainingsMetas().getSize() - 2, url.c_str(), username);

        Serial.printf("active training: %s\n", spiffsLogic.getActiveTraining().getFileName());

        http.begin(url);
        http.addHeader("Content-Type", "application/json");
        size_t trainingsID = 0;
        bool succsess = true;
        DoubleLinkedList<String> uploadedFiles = DoubleLinkedList<String>();
        for (auto &&trainingsMeta : spiffsLogic.getTrainingsMetas())
        {
            if (!spiffsLogic.hasTraining(trainingsMeta.fileName))
                continue;
            if (trainingsMeta.isRunning)
                continue;
            Serial.printf("Uploading %s\n", trainingsMeta.fileName);
            TrainingsSession trainingsSession = spiffsLogic.getTraining(trainingsMeta.fileName);
            const int maxTriggersPerRequest = 5;
            trainingsSession.beginStream();
            bool firstPage = true;
            String sessionName = "";
            while (trainingsSession.hasNext() && succsess)
            {
                JsonBuilder builder = JsonBuilder();
                builder.startObject();
                builder.addKey("user");
                builder.addValue(username);
                if (!firstPage)
                {
                    builder.addKey("sessionName");
                    builder.addValue(sessionName);
                }
                builder.addKey("triggers");
                builder.startArray();
                size_t currentSize = 0;
                while (trainingsSession.hasNext() && currentSize < maxTriggersPerRequest)
                {
                    Trigger trigger = trainingsSession.next();
                    builder.startObject();
                    builder.addKey("triggerType");
                    builder.addValue(trigger.triggerType);
                    builder.addKey("timeMs");
                    builder.addValue(trigger.timeMs);
                    builder.addKey("millimeters");
                    builder.addValue(trigger.millimeters);
                    builder.endObject();
                    currentSize++;
                }
                builder.endArray();
                builder.endObject();
                int httpResponseCode = http.POST(builder.getJson());
                if (httpResponseCode == 200)
                {
                    sessionName = http.getString();
                    firstPage = false;
                    Serial.println("Response from roller results: " + sessionName);
                    sprintf(uploadPopupMessage, "Uploaded %i/%i", trainingsID, spiffsLogic.getTrainingsMetas().getSize());
                    uiManager.handle(true);
                }
                else
                {
                    sprintf(uploadPopupMessage, "Upload error: %i", httpResponseCode);
                    uiManager.handle(true);
                    Serial.printf("Error during upload request. Error code: %i abording upload process\n", httpResponseCode);
                    succsess = false;
                }
            }
            if (succsess)
            {
                uploadedFiles.pushBack(trainingsMeta.fileName);
            }
            else
            {
                break;
            }
            trainingsID++;
        }
        if (succsess)
        {
            for (auto &&fileName : uploadedFiles)
            {
                if (spiffsLogic.deleteSession(fileName))
                {
                    Serial.printf("Deleted %s\n", fileName);
                }
                else
                {
                    Serial.printf("Could not delete session %s\n", fileName);
                }
            }
            sprintf(uploadPopupMessage, "Upload done");
        }
        http.end();
        return true;
    }
    else if (millis() - cloudUploadStart > 5000)
    {
        Serial.printf("Could not connect to %s!\n", uploadWifiSSID);
        return true;
    }
    return false;
}

void tryInitUpload()
{
    if (uploadWifiSSID.length() < 2)
    {
        uiManager.popup("Setup on website first!");
        return;
    }
    if (spiffsLogic.getTrainingsMetas().getSize() <= 1)
    {
        uiManager.popup("Nothing to upload");
        return;
    }
    Serial.printf("Connecting to %s...\n", uploadWifiSSID);
    if (uploadWifiPassword.length() == 0)
    {
        WiFi.begin(uploadWifiSSID);
    }
    else
    {
        WiFi.begin(uploadWifiSSID, uploadWifiPassword);
    }
    cloudUploadStart = millis();
    cloudUploadRunning = true;
}

// void setWiFiActive(bool active) {
//     if(active == wifiRunning) return;
//     if(!active) {
//         endWiFi();
//     }
//     if(active) {
//         beginWiFi();
//     }
//     wifiRunning = active;
// }

void handleWiFi()
{
    if (shouldReboot)
    {
        Serial.println("Rebooting...");
        delay(100);
        ESP.restart();
    }
    if (restardWifiTime && millis() > restardWifiTime)
    {
        uiManager.popup("WiFi updated");
        uiManager.handle(true);
        delay(500);
        uiManager.popup("Rebooting...");
        uiManager.handle(true);
        delay(500);
        ESP.restart();
    }
    // start gun
    if (millis() > startGunTime)
    {
        masterTrigger(Trigger{startGunTime, 0, STATION_TRIGGER_TYPE_START});
        startGunTime = INT32_MAX;
    }
    static bool cloudUploadAttempted = false;
    if (uploadWifiSSID.length() >= 2 && !cloudUploadAttempted && cloudUploadEnabled->isChecked() && spiffsLogic.getTrainingsMetas().getSize() > 1)
    { // dont upload current session
        if (millis() > 10000)
        {
            tryInitUpload();
            cloudUploadAttempted = true;
        }
    }
    if (cloudUploadRunning)
    {
        cloudUploadRunning = !handleCloudUpload();
    }
    static size_t lastTriggerCount = 0;
    if (spiffsLogic.getActiveTraining().getTriggerCount() > lastTriggerCount)
    {
        resolveLiveRequests(spiffsLogic.getActiveTraining().getTriggerCount() - lastTriggerCount);
        lastTriggerCount = spiffsLogic.getActiveTraining().getTriggerCount();
    }
}