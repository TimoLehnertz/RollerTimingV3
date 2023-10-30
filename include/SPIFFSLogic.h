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

  bool operator == (const Trigger& other) {
    return other.timeMs == timeMs && other.millimeters == millimeters && other.triggerType == triggerType;
  }
};
// #endif

#define MAX_TRIGGERS_PER_SESSION

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
  bool isRunning;
};

struct TrainingsSession : public DoubleLinkedList<Trigger> {
public:
  TrainingsSession(): DoubleLinkedList() {
    this->laps = 0;
    this->fileName = String();
    this->filePath = String();
    this->write = false;
    this->isLoaded = false;
    this->lapStarted = false;
  }

  TrainingsSession(String fileName, bool write) : DoubleLinkedList() {
    this->laps = 0;
    this->fileName = fileName;
    this->filePath = String("/") + fileName;
    this->write = write;
    this->isLoaded = write;
    this->lapStarted = false;
  }

  bool loadFromStorage() {
    if(isLoaded) return true;
    clear();
    File file = SPIFFS.open(filePath, FILE_READ, false);
    if(!file) return false;
    while(file.available() >= sizeof(Trigger)) {
      Trigger trigger;
      file.readBytes((char*) &trigger, sizeof(Trigger));
      pushBack(trigger);
    }
    file.close();
    sortTriggers();
    return true;
  }

  void addTrigger(Trigger trigger) {
    if(!write) return;
    File file = SPIFFS.open(filePath, FILE_APPEND, true);
    size_t written = file.write((uint8_t*) &trigger, sizeof(Trigger));
    file.close();
    if(written != sizeof(Trigger)) {
      Serial.println("Failed to write Trigger");
    }
    pushBack(trigger); // add and sort in ram
    sortTriggers();
    if(lapStarted && (trigger.triggerType == STATION_TRIGGER_TYPE_START_FINISH || trigger.triggerType == STATION_TRIGGER_TYPE_FINISH)) {
      laps++;
    }
    if(trigger.triggerType == STATION_TRIGGER_TYPE_START || trigger.triggerType == STATION_TRIGGER_TYPE_START_FINISH) {
      lapStarted = true;
    }
    if(trigger.triggerType == STATION_TRIGGER_TYPE_FINISH) {
      lapStarted = false;
    }
  }

  size_t getFileSize() {
    File file = SPIFFS.open(filePath);
    if(!file) return 0;
    return file.size();
  }

  void sortTriggers() {
    sort(sortCompareTriggers);
  }

  /**
   * Assume list is sorted by time
   * Iterate backwards
   */
  bool triggerExists(const Trigger& trigger) {
    Node<Trigger>* current = tail;
    while(current) {
      if(current->data.timeMs < trigger.timeMs)
        break;
      if(current->data == trigger)
        return true;
      current = current->prev;
    }
    return false;
  }

  size_t getTriggerCount() {
    return getSize();
  }

  timeMs_t getTimeSinceLastTrigger() {
    if(getSize() == 0) return 0;
    return millis() - getLast().timeMs;
  }

  timeMs_t getTimeSinceLastSplit() {
    // return 0;
    if(getSize() == 0) return INT32_MAX;
    Node<Trigger>* current = tail;
    Node<Trigger>* lastFinish = nullptr;
    Node<Trigger>* lastCheckpoint = nullptr;
    while(current) {
      if(lastFinish && current->data.triggerType == STATION_TRIGGER_TYPE_CHECKPOINT) {
        return millis() - lastFinish->data.timeMs;
      }
      if(lastCheckpoint && current->data.triggerType == STATION_TRIGGER_TYPE_CHECKPOINT && current->data.millimeters >= lastCheckpoint->data.millimeters) {
        return INT32_MAX; // same or bigger checkpoint
      }
      if(lastCheckpoint && (current->data.triggerType == STATION_TRIGGER_TYPE_CHECKPOINT || current->data.triggerType == STATION_TRIGGER_TYPE_START || current->data.triggerType == STATION_TRIGGER_TYPE_START_FINISH)) {
        return millis() - lastCheckpoint->data.timeMs;
      }
      if(lastFinish && (current->data.triggerType == STATION_TRIGGER_TYPE_FINISH || current->data.triggerType == STATION_TRIGGER_TYPE_START_FINISH)) {
        return INT32_MAX; // passed already 2 finishes. cant be split time here
      }
      if(current->data.triggerType == STATION_TRIGGER_TYPE_FINISH || current->data.triggerType == STATION_TRIGGER_TYPE_START_FINISH) {
        lastFinish = current;
      }
      if(current->data.triggerType == STATION_TRIGGER_TYPE_CHECKPOINT) {
        lastCheckpoint = current;
      }
      current = current->prev;
    }
    return INT32_MAX;
  }

  timeMs_t getLastSplitTime() {
    // return 0;
    if(getSize() == 0) return 0;
    Node<Trigger>* current = tail;
    Node<Trigger>* lastFinish = nullptr;
    Node<Trigger>* lastCheckpoint = nullptr;
    while(current) {
      if(lastFinish && current->data.triggerType == STATION_TRIGGER_TYPE_CHECKPOINT) { // checkpoint to finish
        return lastFinish->data.timeMs - current->data.timeMs;
      }
      if(lastCheckpoint && current->data.triggerType == STATION_TRIGGER_TYPE_CHECKPOINT && current->data.millimeters >= lastCheckpoint->data.millimeters) {
        return 0;
      }
      if(lastCheckpoint && (current->data.triggerType == STATION_TRIGGER_TYPE_CHECKPOINT || current->data.triggerType == STATION_TRIGGER_TYPE_START || current->data.triggerType == STATION_TRIGGER_TYPE_START_FINISH)) {
        return lastCheckpoint->data.timeMs - current->data.timeMs; // start to checkpoint or checkpoint to checkpoint
      }
      if(current->data.triggerType == STATION_TRIGGER_TYPE_FINISH || current->data.triggerType == STATION_TRIGGER_TYPE_START_FINISH) {
        lastFinish = current;
      }
      if(current->data.triggerType == STATION_TRIGGER_TYPE_CHECKPOINT) {
        lastCheckpoint = current;
      }
      current = current->prev;
    }
    return 0;
  }

  timeMs_t getTimeSinceLastFinish() {
    // return 0;
    if(getSize() == 0) return INT32_MAX;
    Node<Trigger>* current = tail;
    Node<Trigger>* lastFinish = nullptr;
    while(current) {
      if(current->data.triggerType == STATION_TRIGGER_TYPE_FINISH || current->data.triggerType == STATION_TRIGGER_TYPE_START_FINISH) {
        lastFinish = current;
      }
      if(lastFinish && (current->data.triggerType == STATION_TRIGGER_TYPE_START || current->data.triggerType == STATION_TRIGGER_TYPE_START_FINISH)) {
        return millis() - lastFinish->data.timeMs;
      }
      current = current->prev;
    }
    return INT32_MAX;
  }

  bool isLapStarted() {
    return lapStarted;
  }

  timeMs_t getTimeSinceLastStart() {
    if(getSize() == 0) return INT32_MAX;
    Node<Trigger>* current = tail;
    while(current) {
      if(current->data.triggerType == STATION_TRIGGER_TYPE_FINISH) {
        return INT32_MAX;
      }
      if(current->data.triggerType == STATION_TRIGGER_TYPE_START || current->data.triggerType == STATION_TRIGGER_TYPE_START_FINISH) {
        return millis() - current->data.timeMs;
      }
      current = current->prev;
    }
    return INT32_MAX;
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
    if(getSize() == 0) return INT32_MAX;
    Node<Trigger>* current = tail;
    Node<Trigger>* lastFinish = nullptr;
    while(current) {
      if(lastFinish && (current->data.triggerType == STATION_TRIGGER_TYPE_START || current->data.triggerType == STATION_TRIGGER_TYPE_START_FINISH)) {
        return lastFinish->data.timeMs - current->data.timeMs;
      }
      // if(lastFinish && current->data.triggerType == STATION_TRIGGER_TYPE_FINISH) {
      //   return INT32_MAX; // finish after finish cant be a complete lap
      // }
      if(current->data.triggerType == STATION_TRIGGER_TYPE_FINISH || current->data.triggerType == STATION_TRIGGER_TYPE_START_FINISH) {
        lastFinish = current;
      }
      current = current->prev;
    }
    return INT32_MAX;
  }

  timeMs_t getLastLapDistance() {
    if(getSize() == 0) return 0;
    Node<Trigger>* current = tail;
    Node<Trigger>* lastFinish = nullptr;
    while(current) {
      if(lastFinish && (current->data.triggerType == STATION_TRIGGER_TYPE_START || current->data.triggerType == STATION_TRIGGER_TYPE_START_FINISH)) {
        return lastFinish->data.millimeters;
      }
      if(current->data.triggerType == STATION_TRIGGER_TYPE_FINISH || current->data.triggerType == STATION_TRIGGER_TYPE_START_FINISH) {
        lastFinish = current;
      }
      current = current->prev;
    }
    return 0;
  }

  String getFileName() {
    return fileName;
  }
private:
  String fileName;
  String filePath;
  size_t laps;
  bool write;
  bool isLoaded;
  bool lapStarted;
};

class SPIFFSLogic {
public:
  SPIFFSLogic() {
    this->running = false;
    this->activeTraining = TrainingsSession();
    this->activeTrainingsIndex = 0;
    this->trainingsMetas = DoubleLinkedList<TrainingsMeta>();
  }

  bool begin() {
    if (!SPIFFS.begin(true)) {
      Serial.println("SPIFFS Mount Failed");
      return false;
    }
    // SPIFFS.format();
    // File testFile = SPIFFS.open("/test", FILE_APPEND, true);
    // testFile.println("Moin");
    // testFile.close();

    Serial.println("SPIFFS files:");
    File root = SPIFFS.open("/");
    listFiles(root);
    root.close();

    Serial.printf("SPIFFS space: %ikb/%ikb (used: %i%%)\n", SPIFFS.usedBytes() / 1000, SPIFFS.totalBytes() / 1000, int(round(float(SPIFFS.usedBytes()) / float(SPIFFS.totalBytes() + 1) * 100.0)));

    root = SPIFFS.open("/");
    while(File sessionFile = root.openNextFile()) {
      if(sessionFile.size() < sizeof(Trigger)) {
        continue; // skip broken file
      }
      if(!String(sessionFile.name()).endsWith(".rt")) {
        continue;
      }
      TrainingsMeta trainingsMeta = TrainingsMeta();
      trainingsMeta.fileName = String(sessionFile.name());
      trainingsMeta.fileSize = sessionFile.size();
      trainingsMeta.isRunning = false;
      trainingsMetas.pushBack(trainingsMeta);
      sessionFile.close();
    }
    root.close();
    Serial.printf("Found %i session files\n", trainingsMetas.getSize());
    startNewSession();
    running = true;
    return true;
  }

  size_t getBytesTotal() {
    return SPIFFS.totalBytes();
  }

  size_t getBytesUsed() {
    return SPIFFS.usedBytes();
  }

  void addTrigger(const Trigger& trigger) {
    if(!running) return;
    activeTraining.addTrigger(trigger);
    trainingsMetas.get(activeTrainingsIndex).fileSize = activeTraining.getFileSize();
    trainingsMetas.get(activeTrainingsIndex).isRunning = true;
  }

  bool triggerExists(const Trigger& trigger) {
    if(!running) return false;
    return activeTraining.triggerExists(trigger);
  }

  bool startNewSession() {
    if(!running) return false;
    if(activeTraining.getFileSize() > 0) {
      trainingsMetas.pushBack(TrainingsMeta{ activeTraining.getFileSize(), activeTraining.getFileName(), false });
    }
    activeTraining = TrainingsSession(getFileNameForNewTraining(), true);
    activeTrainingsIndex = trainingsMetas.pushBack(TrainingsMeta{ activeTraining.getFileSize(), activeTraining.getFileName(), true });
    return true;
  }

  bool hasTraining(String fileName) {
    TrainingsSession session = TrainingsSession(fileName, false);
    return session.loadFromStorage();
  }

  TrainingsSession getTraining(String fileName) {
    TrainingsSession session = TrainingsSession(fileName, false);
    session.loadFromStorage();
    return session;
  }

  const DoubleLinkedList<TrainingsMeta>& getTrainingsMetas() {
    return trainingsMetas;
  }

  TrainingsSession& getActiveTraining() {
    return activeTraining;
  }

  bool deleteSession(String fileName) {
    File file = SPIFFS.open(String("/") + fileName);
    if(!file) return false;
    bool succsess = SPIFFS.remove(file.path());
    if(!succsess) return false;
    size_t i = 0;
    for (auto &&trainingsMeta : trainingsMetas) {
      if(trainingsMeta.fileName == fileName) {
        trainingsMetas.removeIndex(i);
        if(i == activeTrainingsIndex) {
          return false; // dont delete active training
        }
        if(i < activeTrainingsIndex) {
          activeTrainingsIndex--;
        }
        break;
      }
      i++;
    }
    return true;
  }

  void deleteAllSessions() {
    File root = SPIFFS.open("/");
    while(File sessionFile = root.openNextFile()) {
      if(!String(sessionFile.name()).endsWith(".rt")) {
        continue;
      }
      bool succsess = SPIFFS.remove(sessionFile.path());
      Serial.printf("Deleting %s/%s succsess: %i\n", sessionFile.path(), String(sessionFile.name()), succsess);
    }
    trainingsMetas.clear();
    Serial.println("Deleted all sessions. Updated file system:");
    root.close();
    root = SPIFFS.open("/");
    listFiles(root);
    root.close();
  }

private:
  bool running;
  DoubleLinkedList<TrainingsMeta> trainingsMetas;
  TrainingsSession activeTraining;
  size_t activeTrainingsIndex;

  void listFiles(File& dir, uint8_t intends = 0) {
    if(dir.isDirectory()) {
      while(File f = dir.openNextFile()) {
        if(f.isDirectory()) {
          Serial.printf("%s/\n", f.name());
          listFiles(f, intends + 1);
        } else {
          // if(f.size() > 1024) {
          //   Serial.printf("%s (%ikb)\n", f.name(), round(f.size() / 1024));
          // } else {
            Serial.printf("%s (%ibytes)\n", f.name(), f.size());
          // }
        }
        f.close();
      }
    } else {
      Serial.printf("%s is no directory to print\n", dir.name());
    }
  }

  String getFileNameForNewTraining() {
    int maxId = 0;
    for (TrainingsMeta &trainingsMeta : trainingsMetas) {
      maxId = max(maxId, trainingsMeta.fileName.toInt());
    }
    return String(maxId + 1) + ".rt";
  }
};