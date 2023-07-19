#pragma once
#include <Arduino.h>
#include <Global.h>
#include <SPIFFS.h>
#include <LinkedList.h>

#ifndef TRIGGER_DEFINED
#define TRIGGER_DEFINED

#define STATION_TRIGGER_TYPE_START_FINISH 0
#define STATION_TRIGGER_TYPE_START 1
#define STATION_TRIGGER_TYPE_CHECKPOINT 2
#define STATION_TRIGGER_TYPE_FINISH 3

struct Trigger {
  uint32_t timeMs; // overflows after 50 days
  uint16_t millimeters;
  uint8_t triggerType;
};
#endif

#define MAX_TRIGGERS_PER_SESSION

const char* sessionsPath = "/sessions";

/**
 * Treat every trigger as new lap
 */
#define TRAININGS_TYPE_NONE 0

/**
 * Treat every trigger as new lap
 */
#define TRAININGS_TYPE_NORMAL 1

/**
 * Define the order in wich lasers are located. one cycle through all of them will be a full lap
 */
#define TRAININGS_TYPE_NORMAL_SUB_LAPS 1

int sortCompareTriggers(Trigger& a, Trigger& b) {
  return a.timeMs - b.timeMs;
}

struct SessionMetadata {
  SessionMetadata() : id(0), valid(false), sessionType(TRAININGS_TYPE_NONE) {}
  SessionMetadata(uint16_t id) : id(id), valid(true), sessionType(TRAININGS_TYPE_NORMAL) {}
  uint16_t id; // index of session. Should be unique for all sessions
  bool valid;
  uint8_t sessionType;
  uint8_t setup[20]; // placeholder for future addidions
};

struct TrainingsSession {
public:
  TrainingsSession() : metaData(SessionMetadata()), triggers(LinkedList<Trigger>()), path(String()) {}
  TrainingsSession(uint16_t id) : metaData(SessionMetadata(id)), triggers(LinkedList<Trigger>()) {
    path = String() + sessionsPath + "/" + getFileName();
  }

  void begin() {
    
  }

  void end() {
    
  }

  void addTrigger(uint32_t timeMs, uint16_t millimeters, uint8_t triggerType) {
    if(triggers.size() == 0) { // this is the first trigger
      File file;
      file = SPIFFS.open(path, FILE_WRITE, true);
      size_t size = file.write((uint8_t*) &metaData, sizeof(SessionMetadata));
      if(size != sizeof(SessionMetadata)) {
        Serial.println("Error writing .rrt header");
        return;
      }
      file.close();
    }
    Trigger trigger = Trigger { timeMs, millimeters, triggerType };
    File file = SPIFFS.open(path, FILE_APPEND, true);
    size_t written = file.write((uint8_t*) &trigger, sizeof(Trigger));
    file.close();
    if(written != sizeof(Trigger)) {
      Serial.println("Failed to write Trigger");
    }
    triggers.add(trigger); // add and sort in ram
    triggers.sort(sortCompareTriggers);
  }

  size_t getTriggerCount() {
    return triggers.size();
  }

  uint32_t getTimeSinceLastTrigger() {
    if(triggers.size() == 0) return 0;
    return millis() - triggers.get(triggers.size() - 1).timeMs;
  }

  Trigger getTrigger(size_t index) {
    return triggers.get(index);
  }

  size_t getLapsCount() {
    if(triggers.size() == 0) return 0;
    return triggers.size() - 1;
  }

  uint32_t getLastLapMs() {
    if(triggers.size() < 2) return 0;
    return triggers.get(triggers.size() - 1).timeMs - triggers.get(triggers.size() - 2).timeMs;
  }

  String getFilePath() {
    return path;
  }

  String getFileName() {
    return String(metaData.id) + ".rrt";
  }

  SessionMetadata getSessionMeta() {
    return metaData;
  }

  bool isValid() {
    return metaData.valid;
  }
private:
  SessionMetadata metaData;
  LinkedList<Trigger> triggers;
  String path;
};

class SPIFFSLogic {
public:
  SPIFFSLogic() {
    this->running = false;
    this->sessionMetas = LinkedList<SessionMetadata>();
    this->currentSession = TrainingsSession(); // invalid session
    this->sessionLoaded = false;
  }

  bool begin() {
    if (!SPIFFS.begin(true) && !SPIFFS.begin(true) && !SPIFFS.begin(true)) { // try 3 times in case formating helps
      Serial.println("SPIFFS Mount Failed");
      return false;
    }
    Serial.printf("SPIFFS space: %i/%ikb (used: %i%)\n", SPIFFS.usedBytes() / 1000, SPIFFS.totalBytes() / 1000, round(SPIFFS.totalBytes() / (SPIFFS.usedBytes() + 1) * 100));
    if(!SPIFFS.exists(sessionsPath)) {
      SPIFFS.mkdir(sessionsPath);
      Serial.printf("Created directory %s\n", sessionsPath);
    }

    File root = SPIFFS.open("/");
    listFiles(root);
    root.close();
    
    File sessionsDir = SPIFFS.open(sessionsPath);
    while(File sessionFile = sessionsDir.openNextFile()) {
      if(sessionFile.size() < sizeof(SessionMetadata)) {
        sessionMetas.add(SessionMetadata());
        continue;
      }
      SessionMetadata sessionMeta = SessionMetadata();
      sessionFile.readBytes((char*) &sessionMeta, sizeof(SessionMetadata));
      sessionMetas.add(sessionMeta);
      sessionFile.close();
    }
    sessionsDir.close();
    Serial.printf("Found %i session files\n", sessionMetas.size());
    running = true;
    return true;
  }

  LinkedList<SessionMetadata> getSessions() {
    return sessionMetas;
  }

  bool addTrigger(uint32_t timeMs, uint16_t millimeters, uint8_t triggerType) {
    if(!currentSession.isValid() || !this->sessionLoaded) return false;
    currentSession.addTrigger(timeMs, millimeters, triggerType);
    return true;
  }

  bool startNewSession() {
    if(!running) return false;
    if(sessionLoaded) {
      endSession();
    }
    currentSession = TrainingsSession(getIdForNewSession());
    sessionMetas.add(currentSession.getSessionMeta());
    currentSession.begin();
    this->sessionLoaded = true;
    return true;
  }

  bool endSession() {
    if(!running) return false;
    currentSession.end();
    this->sessionLoaded = false;
    return true;
  }

  TrainingsSession* getCurrentSession() {
    if(!sessionLoaded) return nullptr;
    return &currentSession;
  }

private:
  bool running;
  LinkedList<SessionMetadata> sessionMetas; // read only
  TrainingsSession currentSession;
  bool sessionLoaded;

  void listFiles(File& dir, uint8_t intends = 0) {
    if(!dir.isDirectory()) { // file
      for (size_t i = 0; i < intends; i++) {
        Serial.print("  ");
      }
      Serial.printf("%s (%ibytes)\n", dir.name(), dir.size());
    } else {
      while(File f = dir.openNextFile()) {
        listFiles(f, intends + 1);
        f.close();
      }
    }
  }

  uint16_t getIdForNewSession() {
    uint16_t maxId = 0;
    for (size_t i = 0; i < sessionMetas.size(); i++) {
      maxId = max(maxId, sessionMetas.get(i).id);
    }
    return maxId + 1;
  }
};