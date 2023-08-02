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

struct TrainingsMeta {
  size_t fileSize;
  String fileName;
};

struct TrainingsSession : protected DoubleLinkedList<Trigger> {
public:
  TrainingsSession(): DoubleLinkedList() {
    this->path = String() ;
    this->laps = 0;
    this->filenName = String();
    this->write = false;
    this->isLoaded = false;
  }

  TrainingsSession(String fileName, bool write) : DoubleLinkedList() {
    this->path = String(sessionsPath) + "/" + getFileName();
    this->laps = 0;
    this->filenName = fileName;
    this->write = write;
    this->isLoaded = write;
  }

  bool loadFromStorage() {
    if(isLoaded) return true;
    clear();
    if(!SPIFFS.exists(path)) return false;
    File file = SPIFFS.open(path, FILE_READ, false);
    while(file.available() >= sizeof(Trigger)) {
      Trigger trigger;
      file.readBytes((char*) &trigger, sizeof(Trigger));
      pushBack(trigger);
    }
    return true;
  }

  void addTrigger(Trigger trigger) {
    if(!write) return;
    File file = SPIFFS.open(path, FILE_APPEND, true);
    size_t written = file.write((uint8_t*) &trigger, sizeof(Trigger));
    file.close();
    if(written != sizeof(Trigger)) {
      Serial.println("Failed to write Trigger");
    }
    pushBack(trigger); // add and sort in ram
    sortTriggers();
    if(trigger.triggerType == STATION_TRIGGER_TYPE_START_FINISH || trigger.triggerType == STATION_TRIGGER_TYPE_FINISH) {
      laps++;
    }
  }

  size_t getFileSize() {
    File file = SPIFFS.open(path);
    if(!file) return 0;
    return file.size();
  }

  void sortTriggers() {
    sort(sortCompareTriggers);
  }

  size_t getTriggerCount() {
    return getSize();
  }

  timeMs_t getTimeSinceLastTrigger() {
    if(getSize() == 0) return 0;
    return millis() - getLast().timeMs;
  }

  const Trigger& getTrigger(size_t index) {
    return get(index);
  }

  size_t getLapsCount() {
    return laps;
  }

  /**
   * Looks for all types of triggers
  */
  timeMs_t getLastLapMs() {
    if(getSize() < 2) return 0;
    return getLast().timeMs - get(getSize() - 2).timeMs;
  }

  String getFilePath() {
    return path;
  }

  String getFileName() {
    return filenName;
  }
private:
  String path;
  String filenName;
  size_t laps;
  bool write;
  bool isLoaded;
};

class SPIFFSLogic {
public:
  SPIFFSLogic() {
    this->running = false;
    this->activeTraining = TrainingsSession();
    this->trainingsMetas = DoubleLinkedList<TrainingsMeta>();
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
      if(sessionFile.size() < sizeof(Trigger)) {
        continue; // skip broken file
      }
      TrainingsMeta trainingsMeta = TrainingsMeta();
      trainingsMeta.fileName = String(sessionFile.name());
      trainingsMeta.fileSize = sessionFile.size();
      trainingsMetas.pushBack(trainingsMeta);
      sessionFile.close();
    }
    sessionsDir.close();
    Serial.printf("Found %i session files\n", trainingsMetas.getSize());
    running = true;
    return true;
  }

  void addTrigger(const Trigger& trigger) {
    activeTraining.addTrigger(trigger);
  }

  bool startNewSession() {
    if(!running) return false;
    if(activeTraining.getFileSize() > 0) {
      trainingsMetas.pushBack(TrainingsMeta{ activeTraining.getFileSize(), activeTraining.getFileName() });
    }
    activeTraining = TrainingsSession(getFileNameForNewTraining(), true);
    return true;
  }

  const DoubleLinkedList<TrainingsMeta>& getTrainingsMetas() {
    return trainingsMetas;
  }

  TrainingsSession& getActiveTraining() {
    return activeTraining;
  }

private:
  bool running;
  DoubleLinkedList<TrainingsMeta> trainingsMetas;
  TrainingsSession activeTraining;

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

  String getFileNameForNewTraining() {
    File sessionsDir = SPIFFS.open(String(sessionsPath));
    int maxId = 0;
    for (TrainingsMeta &trainingsMeta : trainingsMetas) {
      maxId = max(maxId, trainingsMeta.fileName.toInt());
    }
    return String(maxId + 1) + ".rt";
  }
};