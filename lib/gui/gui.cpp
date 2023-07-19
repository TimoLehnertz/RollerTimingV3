#include <gui.h>

void frameDelegator(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) {
    if(state->currentRendering >= UIManager::frameSectionsCount) return;
    UIManager::frameSections[state->currentRendering].render(display, state, x, y);
}

Menu::Menu(const char* backBtnText) {
    this->menuItemsCount = 0;
    this->activeItem = 0;
    this->x = 0;
    this->y = 0;
    this->targetX = 0;
    this->targetY = 0;
    this->lpf = 0.4;
    this->gap = 5;
    this->activeItem = 0;
    this->itemFocused = false;
    this->backBtn = Button(backBtnText);
    this->backBtn.setHighlightBack(true);
}

void Menu::render(ScreenDisplay *display, DisplayUiState* state, int16_t x, int16_t y) {
    this->x = this->targetX * lpf + (1.0 - lpf) * this->x;
    this->y = this->targetY * lpf + (1.0 - lpf) * this->y;
    x -= int(this->x);
    y -= int(this->y);
    if(activeItem >= menuItemsCount) {
        activeItem = menuItemsCount; // back button
    }
    for (size_t i = 0; i < menuItemsCount; i++) {
        if(getItem(i)->isHidden()) continue;
        getItem(i)->render(display, state, x, y);
        y += getItem(i)->getHeight() + gap;
    }
    backBtn.render(display, state, x, y);
}

MenuItem* Menu::getFocusedItem() {
    return getItem(activeItem);
}

MenuItem* Menu::getItem(size_t index) {
    if(index < menuItemsCount) {
        return menuItems[index];
    } else {
        return &backBtn;
    }
}

size_t Menu::getItemCount() {
    return menuItemsCount;
}

/**
 * @return true if the result of this event causes this frame section to loose focus
 * @return false if this section will keep its focus
 */
bool Menu::handleEvent(GUIProcessedEvent event) {
    if(itemFocused) {
        bool willLooseFocus = getFocusedItem()->handleEvent(event);
        if(willLooseFocus) {
            closeSubMenu();
            itemFocused = false;
        }
        return false;
    }
    switch(event) {
        case PROCESSED_EVENT_CANCEL: {
            removeFocus();
            return true;
        }
        case PROCESSED_EVENT_ENTER: {
            if(activeItem == menuItemsCount) { // back button clicked
                removeFocus();
                return true;
            }
            bool willLooseFocus = getFocusedItem()->focus();
            if(!willLooseFocus) {
                itemFocused = true;
                if(getFocusedItem()->isSubMenu()) {
                    openSubMenu();
                }
            }
            break;
        }
        case PROCESSED_EVENT_SCROLL_DOWN: {
            for (int16_t newActiveItem = activeItem + 1; newActiveItem <= menuItemsCount; newActiveItem++) { // <= because of back button
                if(getItem(newActiveItem)->isSelectable() && !getItem(newActiveItem)->isHidden()) {
                    getItem(activeItem)->setHighlighted(false);
                    getItem(newActiveItem)->setHighlighted(true);
                    scrollToItem(newActiveItem);
                    activeItem = newActiveItem;
                    break;
                }
            }
            break;
        }
        case PROCESSED_EVENT_SCROLL_UP: {
            for (int16_t newActiveItem = activeItem - 1; newActiveItem >= 0; newActiveItem--) {
                if(getItem(newActiveItem)->isSelectable() && !getItem(newActiveItem)->isHidden()) {
                    getItem(activeItem)->setHighlighted(false);
                    getItem(newActiveItem)->setHighlighted(true);
                    scrollToItem(newActiveItem);
                    activeItem = newActiveItem;
                    break;
                }
            }
            break;
        }
    }
    return false;
}


void Menu::closeSubMenu() {
    targetX = 0;
    scrollToItem(activeItem);
    getFocusedItem()->setHighlighted(true);
}

void Menu::openSubMenu() {
    int16_t itemTop = 0;
    for (size_t i = 0; i < activeItem; i++) {
        if(getItem(i)->isHidden()) continue;
        itemTop += getItem(i)->getHeight() + gap;
    }
    targetY = itemTop;
    targetX = 128;
}

void Menu::scrollToItem(size_t index) {
    int16_t itemTop = 0;
    for (size_t i = 0; i < index; i++) {
        if(getItem(i)->isHidden()) continue;
        itemTop += getItem(i)->getHeight() + gap;
    }
    targetY = itemTop - getItem(index)->getHeight() / 2.0 - 32 + 5;
}

void Menu::removeFocus(){
    if(itemFocused) {
        getFocusedItem()->removeFocus();
    }
    itemFocused = false;
    targetX = 0;
    targetY = 0;
}

void Menu::focus() {
    x = 0;
    getItem(activeItem)->setHighlighted(false);
    while(true) { // trusting that at least the back button is visible and selectable
        if(getItem(activeItem)->isHidden() || !getItem(activeItem)->isSelectable()) {
            activeItem++;
        } else {
            scrollToItem(activeItem);
            getItem(activeItem)->setHighlighted(true);
            break;
        }
    }
}

void Menu::addItem(MenuItem* item) {
    if(menuItemsCount >= MAX_MENU_ITEM_COUT) return;
    menuItems[menuItemsCount++] = item;
}

void Menu::removeItem(MenuItem* item) {
    for (size_t i = 0; i < menuItemsCount; i++) {
        if(menuItems[i] == item) {
            for (size_t l = i; l < menuItemsCount - 1; l++) {
                menuItems[l] = menuItems[l + 1];
            }
            menuItemsCount--;
        }
    }
}

UIManager::UIManager(DisplayUi* displayUI) {
    this->displayUI = displayUI;
    this->nextUIUpdate = 0;
    this->activeSection = 0;
    this->sectionFocused = false;
    this->mouseDownMs = 0;
    this->mouseDown = false;
    this->message = nullptr;
    this->backBtnAction = UINT32_MAX;
}

UIManager::~UIManager() {
    delete[] frameCallbacks;
    this->frameCallbacks = nullptr;
    this->displayUI = nullptr;
    this->overlays = nullptr;
    this->overlayCount = 0;
    this->nextUIUpdate = 0;
    this->activeSection = 0;
    this->sectionFocused = false;
    this->mouseDownMs = 0;
    this->mouseDown = false;
}

void UIManager::begin(OverlayCallback* userOverlays, size_t overlayCount, FrameSection* frameSections1, size_t frameSectionsCount1) {
    this->overlays = overlays;
    this->overlayCount = overlayCount;
    frameSections = frameSections1;
    frameSectionsCount = frameSectionsCount1;
    if(frameSectionsCount > 0) {
        this->frameCallbacks = new FrameCallback[frameSectionsCount];
        for (size_t i = 0; i < frameSectionsCount; i++) {
            frameCallbacks[i] = frameDelegator;
        }
    }

    // The ESP is capable of rendering 60fps in 80Mhz mode
    // but that won't give you much time for anything else
    // run it in 160Mhz mode or just set it to 30 fps
    displayUI->setTargetFPS(60);

    // Customize the active and inactive symbol
    displayUI->setActiveSymbol(activeSymbol);
    displayUI->setInactiveSymbol(inactiveSymbol);

    // You can change this to
    // TOP, LEFT, BOTTOM, RIGHT
    displayUI->setIndicatorPosition(BOTTOM);

    // Defines where the first frame is located in the bar.
    displayUI->setIndicatorDirection(LEFT_RIGHT);

    // You can change the transition that is used
    // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
    displayUI->setFrameAnimation(SLIDE_RIGHT);

    // Add frames
    if(frameSectionsCount > 0) {
        displayUI->setFrames(frameCallbacks, frameSectionsCount);
    }

    displayUI->setTimePerTransition(75);

    OverlayCallback* overlays = new OverlayCallback[this->overlayCount + 1];
    for (size_t i = 0; i < this->overlayCount; i++) {
        overlays[i] = userOverlays[i];
    }
    overlays[this->overlayCount] = popupOverlay;
    
    // Add overlays
    displayUI->setOverlays(overlays, this->overlayCount + 1);

    // Initialising the UI will init the display too.
    static bool displayInitiated = false;
    if(!displayInitiated) {
        displayUI->init();
        displayInitiated = true;
    }

    displayUI->disableAutoTransition();
}

void UIManager::handle() {
    // events
    if(mouseDown && millis() > backBtnAction) {
        handleProcessedEvent(PROCESSED_EVENT_CANCEL);
        backBtnAction = millis() + MAX_ENTER_TIME_MS;
        mouseDownMs = INT64_MAX;
        // mouseDown = false;
    }
    if(millis() > nextUIUpdate) {
        nextUIUpdate = millis() + displayUI->update();
    }
}

void UIManager::triggerInputEvent(GUIInputEvent event) {
    handleProcessedEvent(processEvent(event));
}

void UIManager::popup(const char* message) {
    this->message = message;
}

GUIProcessedEvent UIManager::processEvent(GUIInputEvent event) {
    switch(event) {
        case INPUT_EVENT_MOUSE_DOWN: {
            mouseDown = true;
            mouseDownMs = millis();
            backBtnAction = millis() + MAX_ENTER_TIME_MS;
            return PROCESSED_EVENT_NONE;
        }
        case INPUT_EVENT_MOUSE_UP: {
            mouseDown = false;
            int64_t dt = millis() - mouseDownMs;
            if(dt < MAX_ENTER_TIME_MS && mouseDownMs != INT64_MAX) {
                return PROCESSED_EVENT_ENTER;
            } else {
                return PROCESSED_EVENT_NONE;
            }
            break;
        }
        case INPUT_EVENT_SCROLL_DOWN: {
            if(mouseDown) {
                return PROCESSED_EVENT_NONE;
            } else {
                return PROCESSED_EVENT_SCROLL_DOWN;
            }
        }
        case INPUT_EVENT_SCROLL_UP: {
            if(mouseDown) {
                return PROCESSED_EVENT_NONE;
            } else {
                return PROCESSED_EVENT_SCROLL_UP;
            }
        }
    }
    return PROCESSED_EVENT_NONE;
}

void UIManager::handleProcessedEvent(GUIProcessedEvent event) {
    if(event == PROCESSED_EVENT_NONE) return;
    switch(event) {
        case PROCESSED_EVENT_ENTER:
            beepOK();
            break;
        case PROCESSED_EVENT_CANCEL:
            beepCancel();
            break;
        case PROCESSED_EVENT_SCROLL_UP:
            beepBackwards();
            break;
        case PROCESSED_EVENT_SCROLL_DOWN:
            beepForewards();
            break;
    }
    if(message != nullptr) {
        message = nullptr;
        return;
    }
    if(sectionFocused) {
        if(activeSection >= frameSectionsCount) return;
        bool willLooseFocus = frameSections[activeSection].handleEvent(event);
        if(willLooseFocus) {
            sectionFocused = false;
            displayUI->enableAllIndicators();
        }
    } else {
        switch(event) {
            case PROCESSED_EVENT_ENTER:
                sectionFocused = !frameSections[activeSection].focus();
                if(sectionFocused) {
                    displayUI->disableAllIndicators();
                }
                break;
            case PROCESSED_EVENT_CANCEL:
                displayUI->transitionToFrame(0);
                activeSection = 0;
                break;
            case PROCESSED_EVENT_SCROLL_UP:
                displayUI->nextFrame();
                activeSection = displayUI->getNextFrameNumber();
                break;
            case PROCESSED_EVENT_SCROLL_DOWN:
                displayUI->previousFrame();
                activeSection = displayUI->getNextFrameNumber();
                break;
        }
    }
}

void UIManager::beepForewards() {
    EasyBuzzer.beep(
        5000,		// Frequency in hertz(HZ). 
        10, 	// On Duration in milliseconds(ms).
        100, 	// Off Duration in milliseconds(ms).
        1, 			// The number of beeps per cycle.
        100, 	// Pause duration.
        1 		// The number of cycle.
    );
}

void UIManager::beepBackwards() {
    EasyBuzzer.beep(
        6000,		// Frequency in hertz(HZ). 
        10, 	// On Duration in milliseconds(ms).
        100, 	// Off Duration in milliseconds(ms).
        1, 			// The number of beeps per cycle.
        100, 	// Pause duration.
        1 		// The number of cycle.
    );
}

void UIManager::beepOK() {
    EasyBuzzer.beep(
        4500,		// Frequency in hertz(HZ). 
        20, 	// On Duration in milliseconds(ms).
        100, 	// Off Duration in milliseconds(ms).
        1, 			// The number of beeps per cycle.
        100, 	// Pause duration.
        1 		// The number of cycle.
    );
}

void UIManager::beepCancel() {
    EasyBuzzer.beep(
        4000,		// Frequency in hertz(HZ). 
        50, 	// On Duration in milliseconds(ms).
        100, 	// Off Duration in milliseconds(ms).
        1, 			// The number of beeps per cycle.
        100, 	// Pause duration.
        1 		// The number of cycle.
    );
}

void UIManager::beepReceive() {
    EasyBuzzer.beep(
        8000,		// Frequency in hertz(HZ). 
        20, 	// On Duration in milliseconds(ms).
        100, 	// Off Duration in milliseconds(ms).
        2, 			// The number of beeps per cycle.
        100, 	// Pause duration.
        1 		// The number of cycle.
    );
}

FrameSection* UIManager::frameSections;
size_t UIManager::frameSectionsCount;
const char* UIManager::message;