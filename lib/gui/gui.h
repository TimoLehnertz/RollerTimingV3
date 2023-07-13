#pragma once
#include <HT_DisplayUi.h>
#include <images.h>
#include <EasyBuzzer.h>

#define MAX_ENTER_TIME_MS 1000
#define MAX_MENU_ITEM_COUT 10

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
    }

    MenuItem(bool selectable, int height) {
        this->selectable = selectable;
        this->highlighted = false;
        this->height = height;
        this->hiding = false;
        this->changeListener = nullptr;
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
        display->setFont(ArialMT_Plain_16);
        if(highlighted) {
            display->drawString(x, y, ">");
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

    bool isHiding() {
        return hiding;
    }

    void setChangeListener(ChangeListener changeListener) {
        this->changeListener = changeListener;
    }


protected:
    bool selectable;
    bool highlighted;
    int height;
    bool hiding;
    ChangeListener changeListener;
    
    void change() {
        if(this->changeListener != nullptr) {
            this->changeListener();
        }
    }
};

class TextItem : public MenuItem {
public:
    TextItem(const char* text, bool selectable = false) : MenuItem(selectable, 10) {
        this->text = text;
    }

    void renderComponent(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) override {
        display->setTextAlignment(TEXT_ALIGN_CENTER);
        display->setFont(ArialMT_Plain_10);
        display->drawString(x + 64, y, text);
        // display->drawLine(x + 5, y + 5, x + 10, y + 5);
        // display->drawLine(x + 128 - 10, y + 5, x + 128 - 5, y + 5);
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
        display->drawString(x + 70, y + 3, text);
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
    CheckBox(const char* text, bool checked = false, ChangeListener actionListener = nullptr) : MenuItem(true, 16) {
        this->text = text;
        this->checked = checked;
        this->changeListener = actionListener;
    }

    void renderComponent(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) override {
        display->setTextAlignment(TEXT_ALIGN_LEFT);
        display->setFont(ArialMT_Plain_10);
        display->drawString(x + 14 + 16 + 2, y + 3, text);
        // display->drawRect(x + 14, y, 16, 16);
        display->drawCircle(x + 14 + 8, y + 8 , 8);
        if(checked) {
            // display->drawLine(x + 14 + 2, y + 5, x + 5, y + 14);
            // display->drawLine(x + 14 + 5, y + 13, x + 14, y + 2);
            display->fillCircle(x + 14 + 8, y + 8 , 5);
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

    void setChecked(bool checked) {
        this->checked = checked;
    }

    void removeFocus() {

    }

private:
    const char* text;
    bool checked;
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
        display->drawString(x + 90, y + yOffset, String(number, precision));
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
                number-=step;
                break;
            }
            case PROCESSED_EVENT_SCROLL_UP: {
                number+=step;
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
        this->number = value;
    }

    void removeFocus() {

    }

    void setEditable(bool isEditable) {
        this->isEditable = isEditable;
    }

private:
    const char* text;
    const char* unit;
    double number;
    double min;
    double max;
    double step;
    int precision;
    bool isEditable;
};

class Menu {
public:
    Menu();

    void render(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y);

    bool handleEvent(GUIProcessedEvent event);

    void closeSubMenu();

    void openSubMenu();

    void scrollToItem(size_t index);

    void removeFocus();

    void focus();

    void addItem(MenuItem* item);

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
    MenuItem* getItem(size_t index);
};

class SubMenu : public MenuItem {
public:
    SubMenu(const char* text, Menu* subMenu) : MenuItem(true, 10) {
        this->text = text;
        this->subMenu = subMenu;
        this->opened = false;
    }

    void renderComponent(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) override {
        display->setTextAlignment(TEXT_ALIGN_LEFT);
        display->setFont(ArialMT_Plain_10);
        display->drawString(x + 14, y, text);
        if(opened) {
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
        hiding = true;
        return false;
    }

    void removeFocus() {
        hiding = false;
        opened = false;
    }

private:
    const char* text;
    Menu* subMenu;
    bool opened;
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

    void handle();

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