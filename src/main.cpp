/**
 * @file main.cpp
 * @author Timo Lehnertz
 * @brief Main class for the Roller timing sytem
 * @version 2.0
 * @date 2023-07-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <Arduino.h>
#include <Global.h>
#include <EasyBuzzer.h>
#include <WiFi.h>
#include <definitions.h>
#include <GuiLogic.h>
#include <Storage.h>
#include <radio.h>
#include <rotary.h>
#include <MasterSlaveLogic.h>
#include <WiFiLogic.h>
#include <Sound.h>
#include <DoubleLinkedList.h>
#include <driver/adc.h>

void testMillionTriggers();

// void handleBattery() {
//   float voltageDividerMeasured = analogRead(PIN_VBAT) / 4095.0 * 3.3;
//   float vBatUnfiltered = voltageDividerMeasured * 3.8 + DIODE_VOLTAGE_DROP; // measured * scaling + diode
//   float percentLPF = 0.01;
//   vBat = vBatUnfiltered;
//   batPercent = (vBat - batEmpty) / (batFull - batEmpty) * 100; // vBat to bat%
//   // if(abs(lastBatPercent - batPercent) > 30) {
//   //   percentLPF = 1;
//   // }
//   batPercent = (percentLPF * batPercent) + (1.0 - percentLPF) * lastBatPercent; // Low pass filter
//   if(batPercent < 0) batPercent = 0;
//   if(batPercent > 100) batPercent = 100;
//   lastBatPercent = batPercent;
//   vBatText->setValue(vBat);
//   vBatMeasured->setValue(voltageDividerMeasured);
// }

void trigger() {
  triggerCount++;
  lastTriggerMs = millis();
}

void beginLaser() {

}
void endLaser() {

}

void setup() {
  Serial.begin(115200);
  // simple
  Serial.printf("Roller timing v%s\n", VERSION);
  pinMode(PIN_ROTARY_GND, OUTPUT);
  digitalWrite(PIN_ROTARY_GND, LOW);
  beginSounds();
  pinMode(PIN_LED_WHITE, OUTPUT);
  pinMode(PIN_VBAT, INPUT);
  pinMode(PIN_LASER, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_LASER), trigger, FALLING);
  pinMode(PIN_BUZZER_GND, OUTPUT);
  digitalWrite(PIN_BUZZER_GND, LOW);
  pinMode(PIN_GND_1, OUTPUT);
  digitalWrite(PIN_GND_1, LOW);
  // complex without dependencies
  spiffsLogic.begin();

  pinMode(PIN_ROTARY_3V3, OUTPUT);
  digitalWrite(PIN_ROTARY_3V3, HIGH);
  Rotary.begin();
  beginLEDDisplay();
  beginRadio();
  beginLCDDisplay(); // needed for status variables
  // complex with dependencies
  beginMasterSlaveLogic(); // depends on beginLCDDisplay
  // trigger changes
  beginPreferences(); // depends on beginLCDDisplay
  isDisplayChanged();
  if(isDisplaySelect->getValue()) { // is display
    beginWiFi();
  }

  pinMode(1, INPUT);

  Serial.println("Setup complete");
  /**
   * (Tests)
   */
  // testMillionTriggers();
  // Serial.println("formatting Spiffs");
  // bool succsess = SPIFFS.format();
  // if(succsess) {
  //   Serial.println("Spiffs formatted");
  // }
}

void handleTriggers() {
  if(isDisplaySelect->getValue()) return;
  static timeMs_t lastTimeTriggeredMs = 0;
  static timeMs_t lastTriggerCount = 0;
  if(lastTriggerCount != triggerCount) {
    lastTriggerCount = triggerCount;
    if(lastTriggerMs - lastTimeTriggeredMs < minDelayInput->getValue() * 1000 && lastTimeTriggeredMs != 0) {
      return;
    }
    lastTimeTriggeredMs = millis();
    EasyBuzzer.beep(3800, 20, 100, 1,  100, 1);
    if(!isDisplaySelect->getValue()) {
      slaveTrigger(lastTriggerMs, stationTypeSelect->getValue(), uint16_t(distFromStartInput->getValue() * 1000.0));
    }
  }
}

/**
 * Its probably a good idea to remove file writing in TrainingsSession::addTrigger()
 */
void testMillionTriggers() {
  int triggerType = STATION_TRIGGER_TYPE_START_FINISH; // 0
  uint64_t i = 0;
  while(true) {
    Trigger testTrigger;
    testTrigger.timeMs = millis() * 1000; // scaling up to simulate time passing by fast
    testTrigger.millimeters = 65500;
    testTrigger.triggerType = triggerType;
    triggerType++;
    if(triggerType > STATION_TRIGGER_TYPE_MAX) {
      triggerType = STATION_TRIGGER_TYPE_START_FINISH; // 0
    }
    masterTrigger(testTrigger);
    loop();
    i++;
    if(i % 100 == 0) {
      Serial.printf("%i Triggers tested\n", i);
    }
  }
}

void loop() {
  /**
   * normal loop code
   */
  handleTriggers();
  handleRadioReceive();
  handleRadioSend();
  handleMasterSlaveLogic();
  handleRotary();
  uiManager.handle();
  handleLEDS();
  // handleBattery();
  handleSounds();
  handleStartgun();
  handleWiFi();
  EasyBuzzer.update();
  loops++;
  if(millis() - lastHzMeasuredMs > 1000) {
    loopHz = loops;
    hzText->setValue(loopHz);
    lastHzMeasuredMs = millis();
    loops = 0;
  }

  // adc1_config_width(ADC_WIDTH_BIT_12);
  // adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_0);
  // int val = adc1_get_raw(ADC1_CHANNEL_0);


  // float vbat = 100.0 / (100.0+390.0) * analogRead(1);
  // // float vbat = 100.0 / (100.0+390.0) * val;
  // Serial.printf("vBat: %f, val: %i\n", vbat, analogRead(1));
  // vBat = vbat;
  // float vbat = 100 / (100+390) * VADC_IN1;
}



// --------

// /**
//  * @file WiFiLogic.h
//  * @author Timo Lehnertz
//  * @brief File dealing with everything related to ESP32 WiFi
//  * @version 0.1
//  * @date 2023-07-11
//  * 
//  * @copyright Copyright (c) 2024
//  * 
//  */
// #include <Arduino.h>
// #include <SPI.h>
// #include <Wire.h>
// #include <WiFi.h>
// #include <ESPAsyncWebServer.h>
// #include <AsyncTCP.h>
// #include <DNSServer.h>
// #include <HTTPClient.h>

// #define APSSID_DISPLAY_DEFAULT "RollerTimimg"
// #define APSSID_STATION_DEFAULT "RollerTimimg Station"
// #define APPASSWORD_DEFAULT "Speedskate!"

// String APSsid = APSSID_DISPLAY_DEFAULT;
// String APPassword = APPASSWORD_DEFAULT;

// DNSServer dnsServer;
// AsyncWebServer server(80);

// const byte DNS_PORT = 53;

// // The access points IP address and net mask
// // It uses the default Google DNS IP address 8.8.8.8 to capture all 
// // Android dns requests
// IPAddress apIP(8, 8, 8, 8);
// IPAddress netMsk(255, 255, 255, 0);

// void handleUpdate(AsyncWebServerRequest* request);

// class CaptiveRequestHandler : public AsyncWebHandler {
// public:
//   CaptiveRequestHandler() {
//     server.on("/", HTTP_GET, handleUpdate);
//   }

//   virtual ~CaptiveRequestHandler() {}

//   bool canHandle(AsyncWebServerRequest *request){
//     // request->addInterestingHeader("ANY");
//     return true;
//   }

//   void handleRequest(AsyncWebServerRequest *request) {
    
//   }
// };

// // ?
// String processor(const String& var) {
//   return String();
// }


// void handleUpdate(AsyncWebServerRequest* request) {
//     request->send(200, "text/html", F(R""""(<!DOCTYPE html>
//         <html lang="en">
//         <head>
//             <meta charset="UTF-8">
//             <meta name="viewport" content="width=device-width, initial-scale=1.0">
//             <link rel="icon" type="image/png" href="/logo.png">
//             <title>Roller timing</title>
//         </head>
//         <style>
//         * {
//             font-family: "Arial", sans-serif;
//             color: #333;
//         }
//         body {
//             margin: 0;
//             padding: 1rem;
//             background: linear-gradient(to bottom right, #4287f5, #b243f5);
//             display: flex;
//             justify-content: center;
//             align-items: start;
//             padding-top: 5rem;
//             min-height: calc(100vh - 5rem);
//         }
//         .container {
//             display: flex;
//             justify-content: center;
//             align-items: start;
//             background-color: white;
//             width: 40rem;
//             border-radius: 20px;
//             box-shadow: 0 2px 10px rgba(0, 0, 0, 0.2);
//             padding: 30px;
//         }
//         h1 {
//             font-size: 32px;
//             margin-bottom: 20px;
//         }
//         p {
//             font-size: 16px;
//             color: #555;
//             margin: 0;
//         }
//         .download-btn {
//             border: none;
//             display: inline-block;
//             padding: 10px 10px;
//             background-color: #4287f5;
//             color: white;
//             font-size: 15px;
//             border-radius: 8px;
//             text-decoration: none;
//         }
//         </style>
//         <body>
//             <div class="container">
//                 <div class="content">
//                     <div class="error" id="error-message"></div>
//                     <h1>Roller timing version 2.0</h1>
//                     <br><br><br>
//                     <p>Attention. Updating the firmware will erase <b>all</b> your settings and saved Trainings sessions. Only proceed if that is okay!</p>
//                     <p>Upload procedure:</p>
//                     <ol>
//                     <li>Upload "firmware.bin"</li>
//                     <li>The device will reboot. And WiFi will disconnect. Thats normal. If the device does not restard manually wait 15 seconds then powercycle the device</li>
//                     <li>If neccessary reenable wifi</li>
//                     <li>Connect to this site again</li>
//                     <li>Upload spiffs.bin</li>
//                     <li>The device should reboot again. If so the update process was succsessful</li>
//                     </ol>
//                     <form method='POST' action='/uploadUpdate' enctype='multipart/form-data' id='upload-form'>
//                         <input type='file' name='update' accept=".bin" required>
//                         <input type='submit' value='Update' class='download-btn'>
//                     </form>
//                     <br><hr><br>
//                     <p>
//                         Roller timing<br>
//                         Software version: <span style="color: #24240f">2.0</span><br>
//                         Roller results - From skaters for skaters<br>
//                         by Timo Lehnertz
//                     </p>
//                 </div>
//             </div>
//         </body>
//         </html>)""""));
// }


// void setup() {
//     Serial.begin(115200);
//     Serial.println("Starting WiFi");
//     // WiFi.eraseAP();
//     // WiFi.disconnect(false, true);
//     WiFi.softAPConfig(apIP, apIP, netMsk);
//     WiFi.softAP(APSsid, APPassword);
//     IPAddress ip = WiFi.softAPIP();
//     Serial.print("AP IP address: ");
//     Serial.println(ip);

//     // dnsServer.setErrorReplyCode(DNSReplyCode::NoError); 
//     // dnsServer.start(DNS_PORT, "*", apIP);

//     // server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
//     server.on("/", HTTP_GET, handleUpdate);
//     server.begin();
// }

// void loop() {
//     // dnsServer.processNextRequest();
// }