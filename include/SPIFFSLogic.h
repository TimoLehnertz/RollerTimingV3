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
#define STATION_TRIGGER_TYPE_MAX STATION_TRIGGER_TYPE_NONE

#define MAX_TRIGGER_COUNT_IN_CACHE 52 // 52 to show up in live view as 50 laps

#define TRIGGERS_PER_PAGE 5

struct Trigger {
  timeMs_t timeMs; // overflows after 25 days
  uint16_t millimeters; // maximum is 65.535
  uint8_t triggerType;

  Trigger() : timeMs(0), millimeters(0), triggerType(0) {}

  Trigger(timeMs_t timeMs, uint16_t millimeters, uint8_t triggerType) :
    timeMs(timeMs),
    millimeters(millimeters),
    triggerType(triggerType) {}

  bool operator == (const Trigger& other) {
    return other.timeMs == timeMs && other.millimeters == millimeters && other.triggerType == triggerType;
  }

  operator boolean () {
    return timeMs != 0 || millimeters != 0 || triggerType == 0;
  }
};

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

struct SessionPageInfo {
  size_t pageStart; // The index of the trigger starting the page (MUST be a STARTING Trigger or the first ever Trigger)
  size_t pageEnd; // The index of the trigger ending the page (MUST be a FINISHING Trigger or the most up to date Trigger)
};

struct TrainingsMeta {
  size_t fileSize;
  String fileName;
  bool isRunning;
};

class TrainingsSession {
private:
  String fileName;
  String filePath;
  uint16_t laps;
  bool write;
  bool isLoaded;
  bool lapStarted;
  size_t triggerCount;
  DoubleLinkedList<Trigger> cache;

public:
  TrainingsSession() {
    this->laps = 0;
    this->fileName = String();
    this->filePath = String();
    this->write = false;
    this->isLoaded = false;
    this->lapStarted = false;
    this->triggerCount = 0;
    this->cache = DoubleLinkedList<Trigger>();
  }

  TrainingsSession(String fileName, bool write) {
    this->laps = 0;
    this->fileName = fileName;
    this->filePath = String("/") + fileName;
    this->write = write;
    this->isLoaded = false;
    this->lapStarted = false;
    this->triggerCount = 0;
    this->cache = DoubleLinkedList<Trigger>();
  }

  bool fileExists() {
    File file = SPIFFS.open(filePath, FILE_READ, false);
    bool fileExists = file;
    file.close();
    return fileExists;
  }

  void addTrigger(Trigger trigger) {
    if(!write) return;
    File file = SPIFFS.open(filePath, FILE_APPEND, true);
    size_t written = file.write((uint8_t*) &trigger, sizeof(Trigger));
    file.close();
    if(written != sizeof(Trigger)) {
      Serial.println("Failed to write Trigger");
    }
    cache.pushBack(trigger); // add and sort in ram
    sortCache();
    if(cache.getSize() > MAX_TRIGGER_COUNT_IN_CACHE) {
      cache.removeIndex(0);
    }
    if(lapStarted && (trigger.triggerType == STATION_TRIGGER_TYPE_START_FINISH || trigger.triggerType == STATION_TRIGGER_TYPE_FINISH)) {
      laps++;
    }
    if(trigger.triggerType == STATION_TRIGGER_TYPE_START || trigger.triggerType == STATION_TRIGGER_TYPE_START_FINISH) {
      lapStarted = true;
    }
    if(trigger.triggerType == STATION_TRIGGER_TYPE_FINISH) {
      lapStarted = false;
    }
    triggerCount++;
  }

  size_t getPageCount() {
    beginStream();
    size_t triggerCount = streamFile.available() / sizeof(Trigger);
    size_t pagesAmount = ceil(float(triggerCount) / float(TRIGGERS_PER_PAGE));
    endStream();
    return pagesAmount;
  }

  DoubleLinkedList<SessionPageInfo> getSessionPages() {
    DoubleLinkedList<SessionPageInfo> sessionPages;
    sessionPages.clear();
    beginStream();
    int pageEnd = streamFile.available() / sizeof(Trigger);
    endStream();
    int pageStart = pageEnd - TRIGGERS_PER_PAGE + 1;
    while(pageStart > 0) {
      SessionPageInfo sessionPageInfo = { size_t(pageStart), size_t(pageEnd) };
      sessionPages.pushBack(sessionPageInfo);
      pageEnd -= TRIGGERS_PER_PAGE;
      pageStart -= TRIGGERS_PER_PAGE;
    }
    if(pageEnd > 0) {
      SessionPageInfo firstPage = { 0, size_t(pageEnd) };
      sessionPages.pushBack(firstPage);
    }
    return sessionPages;
  }

  void sortCache() {
    cache.sort(sortCompareTriggers);
  }

  size_t getFileSize() {
    File file = SPIFFS.open(filePath);
    if(!file) return 0;
    return file.size();
  }

  /**
   * Assume list is sorted by time
   * Iterate backwards
   */
  bool triggerInCache(const Trigger& trigger) {
    return cache.includes(trigger);
  }

  size_t getTriggerCount() {
    return triggerCount;
  }

  timeMs_t getTimeSinceLastTrigger() {
    if(cache.getSize() == 0) return 0;
    return millis() - cache.getLast().timeMs;
  }

  timeMs_t getTimeSinceLastSplit() {
    if(cache.getSize() == 0) return INT32_MAX;
    Trigger* lastFinish = nullptr;
    Trigger* lastCheckpoint = nullptr;

    for (auto iterator = cache.rbegin(); iterator != cache.rend(); ++iterator) {
      Trigger& current = *iterator;
      if(lastFinish && current.triggerType == STATION_TRIGGER_TYPE_CHECKPOINT) {
        return millis() - lastFinish->timeMs;
      }
      if(lastCheckpoint && current.triggerType == STATION_TRIGGER_TYPE_CHECKPOINT && current.millimeters >= lastCheckpoint->millimeters) {
        return INT32_MAX; // same or bigger checkpoint
      }
      if(lastCheckpoint && (current.triggerType == STATION_TRIGGER_TYPE_CHECKPOINT || current.triggerType == STATION_TRIGGER_TYPE_START || current.triggerType == STATION_TRIGGER_TYPE_START_FINISH)) {
        return millis() - lastCheckpoint->timeMs;
      }
      if(lastFinish && (current.triggerType == STATION_TRIGGER_TYPE_FINISH || current.triggerType == STATION_TRIGGER_TYPE_START_FINISH)) {
        return INT32_MAX; // passed already 2 finishes. cant be split time here
      }
      if(current.triggerType == STATION_TRIGGER_TYPE_FINISH || current.triggerType == STATION_TRIGGER_TYPE_START_FINISH) {
        lastFinish = &current;
      }
      if(current.triggerType == STATION_TRIGGER_TYPE_CHECKPOINT) {
        lastCheckpoint = &current;
      }
    }
    return INT32_MAX;
  }

  timeMs_t getLastSplitTime() {
    if(cache.getSize() == 0) return INT32_MAX;
    Trigger* lastFinish = nullptr;
    Trigger* lastCheckpoint = nullptr;
    for (auto iterator = cache.rbegin(); iterator != cache.rend(); ++iterator) {
      Trigger& current = *iterator;
      if(lastFinish && current.triggerType == STATION_TRIGGER_TYPE_CHECKPOINT) { // checkpoint to finish
        return lastFinish->timeMs - current.timeMs;
      }
      if(lastCheckpoint && current.triggerType == STATION_TRIGGER_TYPE_CHECKPOINT && current.millimeters >= lastCheckpoint->millimeters) {
        return 0;
      }
      if(lastCheckpoint && (current.triggerType == STATION_TRIGGER_TYPE_CHECKPOINT || current.triggerType == STATION_TRIGGER_TYPE_START || current.triggerType == STATION_TRIGGER_TYPE_START_FINISH)) {
        return lastCheckpoint->timeMs - current.timeMs; // start to checkpoint or checkpoint to checkpoint
      }
      if(current.triggerType == STATION_TRIGGER_TYPE_FINISH || current.triggerType == STATION_TRIGGER_TYPE_START_FINISH) {
        lastFinish = &current;
      }
      if(current.triggerType == STATION_TRIGGER_TYPE_CHECKPOINT) {
        lastCheckpoint = &current;
      }
    }
    return 0;
  }

  timeMs_t getTimeSinceLastFinish() {
    if(cache.getSize() == 0) return INT32_MAX;
    Trigger* lastFinish = nullptr;
    Trigger* lastCheckpoint = nullptr;
    for (auto iterator = cache.rbegin(); iterator != cache.rend(); ++iterator) {
      Trigger& current = *iterator;
      if(current.triggerType == STATION_TRIGGER_TYPE_FINISH || current.triggerType == STATION_TRIGGER_TYPE_START_FINISH) {
        lastFinish = &current;
      }
      if(lastFinish && (current.triggerType == STATION_TRIGGER_TYPE_START || current.triggerType == STATION_TRIGGER_TYPE_START_FINISH)) {
        return millis() - lastFinish->timeMs;
      }
    }
    return INT32_MAX;
  }

  bool isLapStarted() {
    return lapStarted;
  }

  timeMs_t getTimeSinceLastStart() {
    if(cache.getSize() == 0) return INT32_MAX;
    for (auto iterator = cache.rbegin(); iterator != cache.rend(); ++iterator) {
      Trigger& current = *iterator;
      if(current.triggerType == STATION_TRIGGER_TYPE_FINISH) {
        return INT32_MAX;
      }
      if(current.triggerType == STATION_TRIGGER_TYPE_START || current.triggerType == STATION_TRIGGER_TYPE_START_FINISH) {
        return millis() - current.timeMs;
      }
    }
    return INT32_MAX;
  }

  /**
   * Looks for all types of triggers
  */
  timeMs_t getLastLapMs() {
    if(cache.getSize() == 0) return INT32_MAX;
    Trigger* lastFinish = nullptr;
    for (auto iterator = cache.rbegin(); iterator != cache.rend(); ++iterator) {
      Trigger& current = *iterator;
      if(lastFinish && (current.triggerType == STATION_TRIGGER_TYPE_START || current.triggerType == STATION_TRIGGER_TYPE_START_FINISH)) {
        return lastFinish->timeMs - current.timeMs;
      }
      // if(lastFinish && current->data.triggerType == STATION_TRIGGER_TYPE_FINISH) {
      //   return INT32_MAX; // finish after finish cant be a complete lap
      // }
      if(current.triggerType == STATION_TRIGGER_TYPE_FINISH || current.triggerType == STATION_TRIGGER_TYPE_START_FINISH) {
        lastFinish = &current;
      }
    }
    return INT32_MAX;
  }

  timeMs_t getLastLapDistance() {
    if(cache.getSize() == 0) return INT32_MAX;
    Trigger* lastFinish = nullptr;
    for (auto iterator = cache.rbegin(); iterator != cache.rend(); ++iterator) {
      Trigger& current = *iterator;
      if(lastFinish && (current.triggerType == STATION_TRIGGER_TYPE_START || current.triggerType == STATION_TRIGGER_TYPE_START_FINISH)) {
        return lastFinish->millimeters;
      }
      if(current.triggerType == STATION_TRIGGER_TYPE_FINISH || current.triggerType == STATION_TRIGGER_TYPE_START_FINISH) {
        lastFinish = &current;
      }
    }
    return 0;
  }

  size_t getLapsCount() {
    return laps;
  }

  String getFileName() {
    return fileName;
  }

  bool streamHasNextTrigger;
  Trigger streamNextTrigger;
  File streamFile;

  bool beginStream() {
    streamFile = SPIFFS.open(filePath, FILE_READ, true);
    if(!streamFile) {
      streamHasNextTrigger = false;
      return false;
    }
    if(streamFile.available() >= sizeof(Trigger)) {
      streamHasNextTrigger = true;
      streamFile.readBytes((char*) &streamNextTrigger, sizeof(Trigger));
    }
    return true;
  }

  bool hasNext() {
    return streamHasNextTrigger;
  }

  Trigger next() {
    if(!hasNext()) {
      Serial.println("Next trigger does not exist");
      return Trigger();
    }
    Trigger nextTrigger = streamNextTrigger;
    if(streamFile.available() >= sizeof(Trigger)) {
      streamFile.readBytes((char*) &streamNextTrigger, sizeof(Trigger));
      streamHasNextTrigger = true;
    } else {
      streamHasNextTrigger = false;
    }
    return nextTrigger;
  }

  void skip(size_t n) {
    while(n-- > 0 && hasNext()) {
      next();
    }
  }

  void endStream() {
    streamFile.close();
  }

  DoubleLinkedList<Trigger>& getCache() {
    return cache;
  }
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

  bool triggerInCache(const Trigger& trigger) {
    if(!running) return false;
    return activeTraining.triggerInCache(trigger);
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
    return session.fileExists();
  }

  TrainingsSession getTraining(String fileName) {
    TrainingsSession session = TrainingsSession(fileName, false);
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

  static void listAllFiles() {
    File root = SPIFFS.open("/");
    listFiles(root);
    root.close();
  }

private:
  bool running;
  DoubleLinkedList<TrainingsMeta> trainingsMetas;
  TrainingsSession activeTraining;
  size_t activeTrainingsIndex;

  static void listFiles(File& dir, uint8_t intends = 0) {
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