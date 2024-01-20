#pragma once
#include <Arduino.h>

class JsonBuilder {
public:
    JsonBuilder() {
        json = "";
    }

    /**
     * @brief 
     * 
     * @param trigger The trigger to be inserted into JSON
     * @param lapIndex the index of the Trigger
     */
    void insertTriggerObj(Trigger trigger) {
        startObject();
        addKey("type"); // triggerType
        addValue(trigger.triggerType);
        addKey("ms"); // milliseconds
        addValue(trigger.timeMs);
        addKey("mm"); // millimeters
        addValue(trigger.millimeters);
        endObject();
    }

    void startObject() {
        addSeparatorIfNeeded();
        json += "{";
        indentationLevel++;
        needsSeparator = false;
    }

    void endObject() {
        indentationLevel--;
        newlineAndIndent();
        json += "}";
        needsSeparator = true;
    }

    void startArray() {
        addSeparatorIfNeeded();
        json += "[";
        indentationLevel++;
        needsSeparator = false;
    }

    void endArray() {
        indentationLevel--;
        newlineAndIndent();
        json += "]";
        needsSeparator = true;
    }

    void addKey(const String& key) {
        addSeparatorIfNeeded();
        json += "\"" + key + "\":";
        needsSeparator = false;
    }

    void addValue(bool value) {
        addSeparatorIfNeeded();
        json += (value ? "true" : "false");
        needsSeparator = true;
    }

    void addValue(int value) {
        addSeparatorIfNeeded();
        json += String(value);
        needsSeparator = true;
    }

    void addValue(float value) {
        addSeparatorIfNeeded();
        json += String(value);
        needsSeparator = true;
    }

    void addValue(const String& value) {
        addSeparatorIfNeeded();
        json += "\"" + escapeString(value) + "\"";
        needsSeparator = true;
    }

    String getJson() const {
        return json;
    }

private:
    String json;
    int indentationLevel = 0;
    bool needsSeparator = false;

    void addSeparatorIfNeeded() {
        if (needsSeparator) {
            json += ",";
            newlineAndIndent();
        }
    }

    void newlineAndIndent() {
        json += "\n";
        for (int i = 0; i < indentationLevel; i++) {
            json += "    ";
        }
    }

    String escapeString(const String& input) {
        String output = input;
        output.replace("\"", "\\\"");
        output.replace("\\", "\\\\");
        output.replace("/", "\\/");
        output.replace("\b", "\\b");
        output.replace("\f", "\\f");
        output.replace("\n", "\\n");
        output.replace("\r", "\\r");
        output.replace("\t", "\\t");
        // Add other escape sequences if needed
        return output;
    }
};