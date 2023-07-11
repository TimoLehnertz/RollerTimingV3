#pragma once
#include <Arduino.h>
#include <SPIFFS.h>
#include <LinkedList.h>

#define MAX_TRIGGERS_PER_SESSION

const char* sessionsPath = "/sessions";

struct Trigger {
  uint32_t timeMs;
  uint16_t macAddress;
};

struct SessionMetadata {
  SessionMetadata() : id(0), valid(false) {}
  SessionMetadata(uint16_t id) : id(id), valid(true) {}
  uint16_t id; // index of session. Should be unique for all sessions
  bool valid;
};

struct TrainingsSession {
public:
  TrainingsSession() : metaData(SessionMetadata()), triggers(LinkedList<Trigger>()), path(String()) {}
  TrainingsSession(uint16_t id) : metaData(SessionMetadata(id)), triggers(LinkedList<Trigger>()) {
    path = String() + sessionsPath + "/" + getFileName();
  }

  void begin() {
    File file;
    file = SPIFFS.open(path, FILE_WRITE, true);
    size_t size = file.write((uint8_t*) &metaData, sizeof(SessionMetadata));
    file.close();
  }

  void addTrigger(uint32_t timeMs, uint16_t macAddress) {
    Trigger trigger = Trigger { timeMs, macAddress };
    triggers.add(trigger);
    File file = SPIFFS.open(path, FILE_APPEND, true);
    size_t written = file.write((uint8_t*) &trigger, sizeof(Trigger));
    file.close();
    if(written != sizeof(Trigger)) {
      Serial.println("Failed to write Trigger");
    }
  }

  String getFilePath() {
    return path;
  }

  String getFileName() {
    return String(metaData.id);
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
  }

  bool begin() {
    if (!SPIFFS.begin()) {
      Serial.println("SPIFFS Mount Failed");
      return false;
    }
    Serial.printf("%i", atoi("123.rrt"));
    Serial.printf("SPIFFS space: %i/%i bytes\n", SPIFFS.usedBytes(), SPIFFS.totalBytes());

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
    Serial.printf("Found %i sessions files", sessionMetas.size());
    running = true;
    return true;
  }

  LinkedList<SessionMetadata> getSessions() {
    return sessionMetas;
  }

  bool addTrigger(uint32_t timeMs, uint16_t macAddress) {
    if(!currentSession.isValid()) return false;
    currentSession.addTrigger(timeMs, macAddress);
    return true;
  }

  bool startNewSession() {
    currentSession = TrainingsSession(getIdForNewSession());
    sessionMetas.add(currentSession.getSessionMeta());
    currentSession.begin();
  }

private:
  bool running;
  LinkedList<SessionMetadata> sessionMetas; // read only
  TrainingsSession currentSession;

  void listFiles(File& dir, uint8_t intends = 0) {
    if(!dir.isDirectory()) { // file
      for (size_t i = 0; i < intends; i++) {
        Serial.print("  ");
      }
      Serial.printf("%s (%ibytes)", dir.name(), dir.size());
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