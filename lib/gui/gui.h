#pragma once
#include <HT_DisplayUi.h>
#include <images.h>
#include <EasyBuzzer.h>

#ifndef TIME_TYPEDEFS
#define TIME_TYPEDEFS
typedef int32_t timeMs_t;
typedef int64_t timeUs_t;
#endif

#define MAX_ENTER_TIME_MS 500
#define MAX_MENU_ITEM_COUT 50

#define MAX_SELECT_OPTIONS 6

typedef void (*ChangeListener)();

enum GUIInputEvent {
    INPUT_EVENT_SCROLL_UP = 0,
    INPUT_EVENT_SCROLL_DOWN,
    INPUT_EVENT_MOUSE_DOWN,
    INPUT_EVENT_MOUSE_UP,
};

enum GUIProcessedEvent {
    PROCESSED_EVENT_NONE = 0,
    PROCESSED_EVENT_SCROLL_UP,
    PROCESSED_EVENT_SCROLL_DOWN,
    PROCESSED_EVENT_ENTER,
    PROCESSED_EVENT_CANCEL,
};

class MenuItem {
public:
    MenuItem() {
        this->selectable = false;
        this->highlighted = false;
        this->height = 0;
        this->hiding = false;
        this->changeListener = nullptr;
        this->iAmAsubMenu = false;
        this->highlightBack = false;
    }

    MenuItem(bool selectable, int height) {
        this->selectable = selectable;
        this->highlighted = false;
        this->height = height;
        this->hiding = false;
        this->changeListener = nullptr;
        this->iAmAsubMenu = false;
        this->highlightBack = false;
    }

    /**
     * @return true if the result of this event causes this frame section to loose focus
     * @return false if this section will keep its focus
     */
    virtual bool handleEvent(GUIProcessedEvent event) = 0;

    /**
     * @return true if the result of this event causes this frame section to loose focus
     * @return false if this section will keep its focus
     */
    virtual bool focus() = 0;

    virtual void removeFocus() = 0;
    
    void render(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) {
        display->setTextAlignment(TEXT_ALIGN_LEFT);
        display->setFont(ArialMT_Plain_10);
        if(highlighted) {
            if(highlightBack) {
                display->drawString(x + (height - 10) / 2, y, "<");
            } else {
                display->drawString(x + (height - 10) / 2, y, ">");
            }
        }
        renderComponent(display, state, x, y);
    }

    virtual void renderComponent(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) = 0;

    bool isSelectable() {
        return selectable;
    }

    int getHeight() {
        return height;
    }

    bool isHighlighted() {
        return highlighted;
    }

    void setHighlighted(bool highlighted) {
        this->highlighted = highlighted;
    }

    void setHidden(bool hidden) {
        this->hiding = hidden;
    }

    bool isHidden() {
        return hiding;
    }

    void setChangeListener(ChangeListener changeListener) {
        this->changeListener = changeListener;
    }

    bool isSubMenu() {
        return iAmAsubMenu;
    }

    void setHighlightBack(bool highlightBack) {
        this->highlightBack = highlightBack;
    }

protected:
    bool highlightBack;
    bool selectable;
    bool highlighted;
    int height;
    bool hiding;
    bool iAmAsubMenu;
    ChangeListener changeListener;
    
    void change() {
        if(this->changeListener != nullptr) {
            this->changeListener();
        }
    }
};

class TextItem : public MenuItem {
public:
    TextItem(const char* text, bool selectable = false, DISPLAY_TEXT_ALIGNMENT textAlignment = DISPLAY_TEXT_ALIGNMENT::TEXT_ALIGN_CENTER) : MenuItem(selectable, 10) {
        this->text = text;
        this->textAlignment = textAlignment;
    }

    void setText(const char* text) {
        this->text = text;
    }

    void renderComponent(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) override {
        display->setTextAlignment(textAlignment);
        display->setFont(ArialMT_Plain_10);
        switch(textAlignment) {
            case DISPLAY_TEXT_ALIGNMENT::TEXT_ALIGN_CENTER_BOTH: {
                display->drawString(x + 64, y + 5, text);
                break;
            }
            case DISPLAY_TEXT_ALIGNMENT::TEXT_ALIGN_CENTER: {
                display->drawString(x + 64, y, text);
                break;
            }
            case DISPLAY_TEXT_ALIGNMENT::TEXT_ALIGN_LEFT: {
                display->drawString(x + 12, y, text);
                break;
            }
            case DISPLAY_TEXT_ALIGNMENT::TEXT_ALIGN_RIGHT: {
                display->drawString(x + 128 - 2, y, text);
                break;
            }
        }
        
    }

    bool handleEvent(GUIProcessedEvent event) {
        return true;
    }

    bool focus() override {
        return true;
    }

    void removeFocus() {

    }

private:
    const char* text;
    DISPLAY_TEXT_ALIGNMENT textAlignment;
};

class Seperator : public MenuItem {
public:
    Seperator() : MenuItem(false, 1) {}

    void renderComponent(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) override {
        display->drawLine(x, y, x + 128, y);
    }

    bool handleEvent(GUIProcessedEvent event) override {
        return true;
    }

    bool focus() override {
        return true;
    }

    void removeFocus() override {
        
    }
};

class Button : public MenuItem {
public:
    Button() : MenuItem(true, 16){
        this->text = nullptr;
        this->changeListener = nullptr;
    }

    Button(const char* text, ChangeListener actionListener = nullptr) : MenuItem(true, 16) {
        this->text = text;
        this->changeListener = actionListener;
    }

    void renderComponent(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) override {
        display->setTextAlignment(TEXT_ALIGN_CENTER);
        display->setFont(ArialMT_Plain_10);
        display->drawString(x + 16 + 50, y + 2, text);
        display->drawRect(x + 16, y, 100, 16);
    }

    bool handleEvent(GUIProcessedEvent event) {
        return true;
    }

    bool focus() override {
        change();
        return true;
    }

    void removeFocus() {

    }

private:
    const char* text;
};

class CheckBox : public MenuItem {
public:
    CheckBox(const char* text, bool round, bool checked = false, ChangeListener actionListener = nullptr) : MenuItem(true, 10) {
        this->text = text;
        this->checked = checked;
        this->round = round;
        this->changeListener = actionListener;
    }

    void renderComponent(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) override {
        display->setTextAlignment(TEXT_ALIGN_LEFT);
        display->setFont(ArialMT_Plain_10);
        display->drawString(x + 14 + 10, y + 0, text);
        if(round) {
            if(checked) {
                display->fillCircle(x + 12 + 4, y + 5 , 5);
            } else {
                display->drawCircle(x + 12 + 4, y + 5 , 5);
            }
        } else {
            if(checked) {
                display->fillRect(x + 12 + 1, y + 2, 8, 8);
            } else {
                display->drawRect(x + 12, y + 1, 10, 10);
            }
        }
    }

    bool handleEvent(GUIProcessedEvent event) {
        return true;
    }

    bool focus() override {
        checked = !checked;
        change();
        return true;
    }

    bool isChecked() {
        return checked;
    }

    void setChecked(bool checked, bool silent = true) {
        bool checkedBefore = this->checked;
        this->checked = checked;
        if(!silent && checkedBefore != checked) {
            change();
        }
    }

    void removeFocus() {

    }

private:
    const char* text;
    bool checked;
    bool round;
};

class NumberField : public MenuItem {
public:
    NumberField(const char* text, const char* unit, double step, double min = INT_MIN, double max = INT_MAX, int precision = 0, double number = 0, ChangeListener changeListener = nullptr) : MenuItem(true, 14) {
        this->text = text;
        this->unit = unit;
        this->number = number;
        this->min = min;
        this->max = max;
        this->step = step;
        this->precision = precision;
        this->changeListener = changeListener;
        this->isEditable = true;
        this->consecutiveIncrements = 0;
        this->consecutiveDecrements = 0;
        this->lastIncrement = 0;
        this->lastDecrement = 0;
        if(strlen(text) > 15) {
            height = 22;
        }
    }

    void renderComponent(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) override {
        display->setTextAlignment(TEXT_ALIGN_LEFT);
        display->setFont(ArialMT_Plain_10);
        int yOffset = 0;
        if(strlen(text) > 15) {
            yOffset = 7;
        }
        display->drawStringMaxWidth(x + 14 + 2, y, 66, text);
        display->drawString(x + 128 - 16, y + yOffset, unit);
        display->setTextAlignment(TEXT_ALIGN_RIGHT);
        display->drawString(x + 128 - 16 - 2, y + yOffset, String(number, precision));
        if(isHighlighted()) {
            display->drawLine(x + 88, y + 12 + yOffset, x + 88 + 20, y + 12 + yOffset);
        }
    }

    bool handleEvent(GUIProcessedEvent event) {
        switch(event) {
            case PROCESSED_EVENT_CANCEL:
            case PROCESSED_EVENT_ENTER:
                change();
                return true;
            case PROCESSED_EVENT_SCROLL_DOWN: {
                if(millis() - lastDecrement < 300) {
                    consecutiveDecrements++;
                } else {
                    consecutiveDecrements = 0;
                }
                double adjustedStep = step;
                if(consecutiveDecrements > 5) {
                    adjustedStep = step * 10;
                }
                lastDecrement = millis();
                number-=adjustedStep;
                break;
            }
            case PROCESSED_EVENT_SCROLL_UP: {
                if(millis() - lastIncrement < 300) {
                    consecutiveIncrements++;
                } else {
                    consecutiveIncrements = 0;
                }
                double adjustedStep = step;
                if(consecutiveIncrements > 5) {
                    adjustedStep = step * 10;
                }
                lastIncrement = millis();
                number+=adjustedStep;
                break;
            }
        }
        if(number < min) number = min;
        if(number > max) number = max;
        return false;
    }

    bool focus() override {
        return !isEditable;
    }

    double getValue() {
        return number;
    }

    void setValue(double value) {
        if(value < this->min) value = this->min;
        if(value > this->max) value = this->max;
        this->number = value;
    }

    void removeFocus() {

    }

    void setEditable(bool isEditable) {
        this->isEditable = isEditable;
    }

protected:
    const char* text;
    const char* unit;
    double number;
    double min;
    double max;
    double step;
    int precision;
    bool isEditable;
    size_t consecutiveIncrements;
    size_t consecutiveDecrements;
    timeMs_t lastIncrement;
    timeMs_t lastDecrement;
};

class TimeInput : public NumberField {
public:
    TimeInput(const char* text, double minMs, double maxMs, double stepMs, double ms = 0, ChangeListener changeListener = nullptr) : NumberField(text, "s", stepMs, minMs, maxMs, 0, ms, changeListener) {}

    void renderComponent(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) override {
        display->setTextAlignment(TEXT_ALIGN_LEFT);
        display->setFont(ArialMT_Plain_10);
        int yOffset = 0;
        if(strlen(text) > 15) {
            yOffset = 7;
        }
        display->drawStringMaxWidth(x + 14 + 2, y, 66, text);
        display->setTextAlignment(TEXT_ALIGN_RIGHT);
        char timeStr[10];
        if(number < 1000) {
            sprintf(timeStr, "%ims", int(number));
        } else if(number < 60000) {
            sprintf(timeStr, "%.3fs", number / 1000.0);
        } else {
            sprintf(timeStr, "%i:%.3f", int(number / 60000.0), (int(number) % 60000) / 1000.0);
        }

        display->drawString(x + 128 - 2, y + yOffset, timeStr);
        if(isHighlighted()) {
            display->drawLine(x + 88, y + 12 + yOffset, x + 88 + 20, y + 12 + yOffset);
        }
    }
};

class Menu {
public:
    Menu(const char* backBtnText = "< Back  ");

    void render(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y);

    bool handleEvent(GUIProcessedEvent event);

    void closeSubMenu();

    void openSubMenu();

    void scrollToItem(size_t index);

    void removeFocus();

    void focus();

    void addItem(MenuItem* item);

    void prependItem(MenuItem* item, bool autoscroll);

    void removeItem(MenuItem* item);

    void removeAll();

    MenuItem* getItem(size_t index);

    size_t getItemCount();
private:
    Button backBtn;
    MenuItem* menuItems[MAX_MENU_ITEM_COUT];
    int16_t menuItemsCount;
    int16_t activeItem;
    double x;
    double y;
    double targetX;
    double targetY;
    double lpf;
    double gap;
    bool itemFocused;

    MenuItem* getFocusedItem();
};

class SubMenu : public MenuItem {
public:
    SubMenu(const char* text) : MenuItem(true, 10) {
        this->text = text;
        this->subMenu = nullptr;
        this->opened = false;
        this->iAmAsubMenu = true;
    }

    SubMenu(const char* text, Menu* subMenu) : MenuItem(true, 10) {
        this->text = text;
        this->subMenu = subMenu;
        this->opened = false;
        this->iAmAsubMenu = true;
    }

    void renderComponent(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) override {
        display->setTextAlignment(TEXT_ALIGN_LEFT);
        display->setFont(ArialMT_Plain_10);
        display->drawString(x + 14, y, text);
        if(opened && subMenu) {
            subMenu->render(display, state, x + 128, y);
        }
    }

    bool handleEvent(GUIProcessedEvent event) {
        if(!subMenu) return true;
        bool willLooseFocus = subMenu->handleEvent(event);
        if(willLooseFocus) {
            removeFocus();
        }
        return willLooseFocus;
    }

    bool focus() override {
        if(!subMenu) return true;
        opened = true;
        subMenu->focus();
        return false;
    }

    void removeFocus() {
        opened = false;
    }

    void setSubMenu(Menu* subMenu) {
        this->subMenu = subMenu;
    }

private:
    const char* text;
    Menu* subMenu;
    bool opened;
};

class Select : public SubMenu {
public:
    Select(const char* name, ChangeListener changeListener = nullptr) : SubMenu(name) {
        this->changeListener = changeListener;
        this->checkboxesSize = 0;
        this->value = 0;
        this->menu = new Menu();
        menu->addItem(new TextItem(name));
        setSubMenu(menu);
    }

    ~Select() {
        delete this->menu;
    }
    
    /**
     * @param name will be displayed as option inside the select
     * @param value will be displayed if scolling by should be a short hand for name
    */
    void addOption(const char* name, const char* value) {
        if(checkboxesSize >= MAX_SELECT_OPTIONS) return;
        CheckBox* checkbox = new CheckBox(name, true, checkboxesSize == 0); // check first checkbox
        values[checkboxesSize] = value;
        checkboxes[checkboxesSize] = checkbox;
        checkboxesSize++;
        menu->addItem(checkbox);
    }

    const char* getSelectedShort() {
        if(checkboxesSize == 0) return nullptr;
        return values[value];
    }

    void setValue(uint8_t value) {
        if(value >= checkboxesSize) return;
        for (size_t i = 0; i < checkboxesSize; i++) {
            checkboxes[i]->setChecked(false);
        }
        checkboxes[value]->setChecked(true);
        this->value = value;
    }

    uint8_t getValue() {
        return value;
    }

    void renderComponent(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) override {
        SubMenu::renderComponent(display, state, x, y);
        if(checkboxesSize == 0) return;
        display->setTextAlignment(TEXT_ALIGN_RIGHT);
        display->setFont(ArialMT_Plain_10);
        display->drawString(x + 128 - 2, y, values[value]);
    }

    bool handleEvent(GUIProcessedEvent event) {
        bool finished = SubMenu::handleEvent(event);
        for (size_t i = 0; i < checkboxesSize; i++) { // find the first checkbox that is not the previous selected one
            if(i == value) continue;
            if(checkboxes[i]->isChecked()) {
                value = i;
                setValue(i); // uncheck all others
                removeFocus(); // close submenu
                change(); // trigger change listener
                return true; // give back focus
            }
        }
        // fallback return to last know state
        setValue(value);
        if(event == GUIProcessedEvent::PROCESSED_EVENT_ENTER) {
            removeFocus();
            return true;
        }
        return finished;
    }

private:
    Menu* menu;
    CheckBox* checkboxes[MAX_SELECT_OPTIONS];
    const char* values[MAX_SELECT_OPTIONS];
    uint8_t checkboxesSize;
    uint8_t value;
};

class UIManager;

class FrameSection {
public:
    FrameSection() {
        this->frameCallback = nullptr;
        this->menu = nullptr;
        this->x = 0;
        this->y = 0;
        this->targetX = 0;
        this->targetY = 0;
        this->focused = false;
        this->lpf = 0.1;
    }

    FrameSection(FrameCallback frameCallback, Menu* menu) {
        this->frameCallback = frameCallback;
        this->menu = menu;
        this->x = 0;
        this->y = 0;
        this->targetX = 0;
        this->targetY = 0;
        this->focused = false;
        this->lpf = 0.4;
    }

    void render(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) {
        this->x = this->targetX * lpf + (1.0 - lpf) * this->x;
        this->y = this->targetY * lpf + (1.0 - lpf) * this->y;
        x -= int(this->x);
        y -= int(this->y);
        if(y <= -1) {
            menu->render(display, state, x, y + 64);
        }
        if(y > -63) {
            frameCallback(display, state, x, y);
        }
    }

    /**
     * @return true if the result of this event causes this frame section to loose focus
     * @return false if this section will keep its focus
     */
    bool handleEvent(GUIProcessedEvent event) {
        if(menu) {
            bool willLooseFocus = menu->handleEvent(event);
            if(willLooseFocus) {
                removeFocus();
            }
            return willLooseFocus;
        } else {
            return true;
        }
    }
    
    /**
     * @return true if the result of this event causes this frame section to loose focus
     * @return false if this section will keep its focus
     */
    bool focus() {
        if(!menu) return true;
        menu->focus();
        this->focused = true;
        this->targetY = 64;
        this->targetX = 0;
        return false;
    }

    void removeFocus() {
        if(menu) menu->removeFocus();
        this->targetY = 0;
        this->targetX = 0;
        this->focused = false;
    }

    void setMenu(Menu* menu) {
        this->menu = menu;
    }

private:
    FrameCallback frameCallback;
    Menu* menu;
    double x;
    double y;
    double targetX;
    double targetY;
    bool focused;
    double lpf;
};

class UIManager {
public:
    static FrameSection* frameSections;
    static size_t frameSectionsCount;
    
    UIManager(DisplayUi* displayUI);

    ~UIManager();

    void begin(OverlayCallback* overlays = nullptr, size_t overlayCount = 0, FrameSection* frameSections1 = nullptr, size_t frameSectionsCount1 = 0);

    void handle(bool forceUpdate = false);

    void triggerInputEvent(GUIInputEvent event);

    void popup(const char* message);

private:
    DisplayUi* displayUI;
    uint64_t nextUIUpdate;
    OverlayCallback* overlays;
    size_t overlayCount;
    FrameCallback* frameCallbacks;
    int activeSection;
    bool sectionFocused;
    bool mouseDown;
    int64_t mouseDownMs;
    timeMs_t backBtnAction;
    static const char* message;

    GUIProcessedEvent processEvent(GUIInputEvent event);

    void handleProcessedEvent(GUIProcessedEvent event);

    void beepForewards();

    void beepBackwards();
    
    void beepOK();
    
    void beepCancel();
    
    void beepReceive();

    static void popupOverlay(ScreenDisplay *display,  DisplayUiState* state) {
        if(message == nullptr) return;
        display->setColor(BLACK);
        display->fillRect(0,0,128,64);
        display->setColor(WHITE);
        display->setFont(ArialMT_Plain_16);
        display->setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
        display->drawStringMaxWidth(128 / 2, 64 / 2, 128, message);
    }
};