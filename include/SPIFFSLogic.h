#pragma once
#include <Arduino.h>
#include <Global.h>
#include <SPIFFS.h>
#include <DoubleLinkedList.h>

#ifndef TIME_TYPEDEFS
#define TIME_TYPEDEFS
typedef int32_t timeMs_t;
typedef int64_t timeUs_t;
#endif

#define STATION_TRIGGER_TYPE_START_FINISH 0
#define STATION_TRIGGER_TYPE_START 1
#define STATION_TRIGGER_TYPE_CHECKPOINT 2
#define STATION_TRIGGER_TYPE_FINISH 3
#define STATION_TRIGGER_TYPE_NONE 4

// #ifndef TRIGGER_DEFINED
// #define TRIGGER_DEFINED
struct Trigger {
  timeMs_t timeMs; // overflows after 25 days
  uint16_t millimeters;
  uint8_t triggerType;
};
// #endif

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

bool sortCompareTriggers(const Trigger& a, const Trigger& b) {
  return a.timeMs < b.timeMs;
}

struct SessionMetadata {
  SessionMetadata() : id(0), valid(false) {}
  SessionMetadata(uint16_t id) : id(id), valid(true) {}
  uint16_t id; // index of session. Should be unique for all sessions
  bool valid;
  union {
    uint8_t data[20]; // reserved
    struct {
      
    };
  };
};

struct TrainingsSession {
public:
  TrainingsSession() : metaData(SessionMetadata()), triggers(DoubleLinkedList<Trigger>()), path(String()) {}
  TrainingsSession(uint16_t id) : metaData(SessionMetadata(id)), triggers(DoubleLinkedList<Trigger>()) {
    path = String() + sessionsPath + "/" + getFileName();
  }

  void begin() {
    
  }

  void end() {
    
  }

  void addTrigger(timeMs_t timeMs, uint16_t millimeters, uint8_t triggerType) {
    if(triggers.getSize() == 0) { // this is the first trigger
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
    triggers.pushBack(trigger); // add and sort in ram
    sortTriggers();
  }

  void sortTriggers() {
    triggers.sort(sortCompareTriggers);
  }

  size_t getTriggerCount() {
    return triggers.getSize();
  }

  timeMs_t getTimeSinceLastTrigger() {
    if(triggers.getSize() == 0) return 0;
    return millis() - triggers.getLast()->timeMs;
  }

  Trigger* getTrigger(size_t index) {
    try  {
      return triggers.get(index);
    } catch(const std::out_of_range& e) {
      return nullptr;
    }
  }

  size_t getLapsCount() {
    if(triggers.getSize() == 0) return 0;
    return triggers.getSize() - 1;
  }

  timeMs_t getLastLapMs() {
    if(triggers.getSize() < 2) return 0;
    return triggers.get(triggers.getSize() - 1)->timeMs - triggers.get(triggers.getSize() - 2)->timeMs;
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
  DoubleLinkedList<Trigger> triggers;
  String path;
};

class SPIFFSLogic {
public:
  SPIFFSLogic() {
    this->running = false;
    this->sessionMetas = DoubleLinkedList<SessionMetadata>();
    this->currentSession = TrainingsSession(); // invalid session
    this->sessionLoaded = false;
  }

  bool begin() {
    if (!SPIFFS.begin(true) && !SPIFFS.begin(true) && !SPIFFS.begin(true)) { // try 3 times in case formating helps
      Serial.println("SPIFFS Mount Failed");
      return false;
    }
    if(!SPIFFS.exists(sessionsPath)) {
      if(SPIFFS.mkdir(sessionsPath)) {
        Serial.printf("Created directory %s\n", sessionsPath);
      }
    }

    File root = SPIFFS.open("/");
    listFiles(root);
    root.close();

    Serial.printf("SPIFFS space: %ikb/%ikb (used: %i%%)\n", SPIFFS.usedBytes() / 1000, SPIFFS.totalBytes() / 1000, int(round(float(SPIFFS.usedBytes()) / float(SPIFFS.totalBytes() + 1) * 100.0)));
    
    
    File sessionsDir = SPIFFS.open(sessionsPath);
    while(File sessionFile = sessionsDir.openNextFile()) {
      if(sessionFile.size() < sizeof(SessionMetadata)) {
        sessionMetas.pushBack(SessionMetadata());
        continue;
      }
      SessionMetadata sessionMeta = SessionMetadata();
      sessionFile.readBytes((char*) &sessionMeta, sizeof(SessionMetadata));
      sessionMetas.pushBack(sessionMeta);
      sessionFile.close();
    }
    sessionsDir.close();
    Serial.printf("Found %i session files\n", sessionMetas.getSize());
    running = true;
    return true;
  }

  bool addTrigger(timeMs_t timeMs, uint16_t millimeters, uint8_t triggerType) {
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
    sessionMetas.pushBack(currentSession.getSessionMeta());
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

  DoubleLinkedList<SessionMetadata>* getSessionMetas() {
    return &sessionMetas;
  }

private:
  bool running;
  DoubleLinkedList<SessionMetadata> sessionMetas; // read only
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
    for (auto &sessionMeta : sessionMetas) {
      maxId = max(maxId, sessionMeta.id);
    }
    
    // for (size_t i = 0; i < sessionMetas.getSize(); i++) {
    //   maxId = max(maxId, sessionMetas.get(i).id);
    // }
    return maxId + 1;
  }
};