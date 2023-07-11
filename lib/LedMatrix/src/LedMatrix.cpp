#include <Arduino.h>
#include "LedMatrix.h"
#include <FastLED.h>
#include "fonts.h"

void swap(int& a, int& b) {
    int tmp = b;
    b = a;
    a = tmp;
}

LedMatrix::LedMatrix() {
    this->leds = 0;
    this->numLeds = 0;
    this->pixelConverter = 0;
    this->width = 0;
    this->height = 0;
    this->blur = 0;
}

LedMatrix::LedMatrix(CRGB* leds, size_t numLeds, PixelConverter pixelConverter, size_t width, size_t height) {
    this->leds = leds;
    this->numLeds = numLeds;
    this->pixelConverter = pixelConverter;
    this->width = width;
    this->height = height;
    this->blur = 0;
}

void LedMatrix::setPixel(int x, int y, CRGB color, bool additive) {
    if(x < 0 || x >= width || y < 0 || y >= height) return;
    uint16_t index = pixelConverter(x, y);
    if(index < numLeds) {
        leds[index] = leds[index] + color * additive;
    }
}

void LedMatrix::renderPixel(int x, int y, CRGB color) {
    int blurry = 1.0 + blur * 4.0;
    color *= 1 - blur * 0.5;
    setPixel(x, y, color, blur == 0);
    for (double i = 1; i < blur + 1; i++) {
        color *= 0.7;
        setPixel(x + i, y + 0, color, true);
        setPixel(x - i, y + 0, color, true);
        setPixel(x + 0, y + i, color, true);
        setPixel(x + 0, y - i, color, true);
    }
}

int LedMatrix::print(const char* str, int x, int y, CRGB color, uint8_t settings, int maxWidth, int offset) {
    uint8_t len = strlen(str);
    int startX = x;
    x += offset;
    int prevX = x;
    for (size_t strpos = 0; strpos < len; strpos++) {
        int nextStartX = x + boundsFromChar(str[strpos], settings);
        if(prevX < startX && nextStartX >= startX) {
            x = print(str[strpos], x, y, color, settings, startX - x, maxWidth);
        } else if(nextStartX < startX) {
            x = nextStartX;
            continue;
        } else {
            x = print(str[strpos], x, y, color, settings, 0, maxWidth);
        }
        maxWidth -= x - max(startX, prevX);
        prevX = x;
        if(maxWidth < 0) return -1;
    }
    return x;
}

void LedMatrix::printSlideOverflow(const char* str, int x, int y, int maxX, CRGB color, uint8_t settings) {
    int overflow = textWidth(str, settings) - (maxX - x + 1);
    double progress = (sin(millis() / 750.0) + 1.0) / 2.0;
    int offset = round(-progress * max(0, overflow));
    print(str, x, y, color, settings, (maxX - x + 1), offset);
}

void LedMatrix::printSlideOverflow(char str, int x, int y, int maxX, CRGB color, uint8_t settings) {
    char arr[2] = {str, 0};
    printSlideOverflow(arr, x, y, maxX, color, settings);
}

int LedMatrix::print(char digit, int xPos, int yPos, CRGB color, uint8_t settings, uint8_t xStart, uint8_t maxWidth) {
    if(digit < ' ' && digit != '\t') {
        return xPos;
    }
    if((settings & UPPERCASE) && digit >= 'a' && digit <= 'z') {
        digit -= 32;
    }
    uint8_t index = digit - ASCII_OFFSET;
    uint8_t width, height;
    const uint8_t* letter;
    if((settings & FONT_SIZE_SMALL) > 0) {
        letter = fontSmall[index];
        width = FONT_SMALL_WIDTH;
        height = FONT_SMALL_HEIGHT;
    } else {
        letter = fontNormal[index];
        width = FONT_NORMAL_WIDTH;
        height = FONT_NORMAL_HEIGHT;
    }

    if(digit == '\t') {
        return xPos + 2 * width; // tab
    }
    if(digit == ' ') {
        return xPos + (width / 2);
    }

    if(!(settings & MONOSPACE)) while(!letter[width - 1]) width--;

    for (int x = xStart; x < min(width, maxWidth); x++) {
        uint8_t column = letter[width - x - 1];
        for (int y = 0; y < height; y++) {
            if(column & (0b00000001 << (height - y - 1))) {
                renderPixel(x + xPos, y + yPos, color);
            }
        }
    }
    if(settings & UNDERLINE) {
        for (int x = xStart; x < min(width + (settings & SPACING), (int) maxWidth); x++) {
            renderPixel(x + xPos, yPos + height + 1, color);
        }
    }
    // if(digit == '.' || digit == ',') {
    //     width--;
    // }
    return xPos + width + (settings & SPACING);
}

void LedMatrix::line(double x1, double y1, double x2, double y2, CRGB color) {
    line((int) round(x1), (int) round(y1), (int) round(x2), (int) round(y2), color);
}

void LedMatrix::line(int x1, int y1, int x2, int y2, CRGB color) {
    if(x1 == x2 && y1 == y2) { // prevent divide by 0
        renderPixel(x1, y1, color);
        return;
    }
    int dx = x2 - x1;
    int dy = y2 - y1;
    if(abs(dx) > abs(dy)) {
        if(x2 < x1) {
            swap(x1, x2);
            swap(y1, y2);
        }
        for (int x = x1; x <= x2; x++) {
            int y = y1 + dy * (x - x1) / dx;
            renderPixel(x, y, color);
        }
    } else {
        if(y2 < y1) {
            swap(x1, x2);
            swap(y1, y2);
        }
        for (int y = y1; y <= y2; y++) {
            int x = x1 + dx * (y - y1) / dy;
            renderPixel(x, y, color);
        }
    }
}

void LedMatrix::rect(int x1, int y1, int x2, int y2, CRGB color) {
    line(x1, y1, x1, y2, color);
    line(x1, y1, x2, y1, color);
    line(x2, y2, x1, y2, color);
    line(x2, y2, x2, y1, color);
}

void LedMatrix::dot(int x, int y, CRGB color) {
    renderPixel(x, y, color);
}

int LedMatrix::boundsFromChar(char digit, uint8_t settings) {
    if(digit < ' ' && digit != '\t') {
        return 0;
    }
    if((settings & UPPERCASE) && digit >= 'a' && digit <= 'z') {
        digit -= 32;
    }
    uint8_t index = digit - ASCII_OFFSET;
    uint8_t width;
    const uint8_t* letter;
    if(settings & FONT_SIZE_SMALL) {
        letter = fontSmall[index];
        width = FONT_SMALL_WIDTH;
    } else {
        letter = fontNormal[index];
        width = FONT_NORMAL_WIDTH;
    }
    if(digit == '\t') {
        return 2 * width; // tab
    }
    if(digit == ' ') {
        return width / 2;
    }
    if(!(settings & MONOSPACE)) while(!letter[width - 1]) width--;
    // if(digit == '.' || digit == ',') {
    //     width--;
    // }
    return width + (settings & SPACING);
}

int LedMatrix::textWidth(const char* str, uint8_t settings) {
    uint8_t len = strlen(str);
    int width = 0;
    for (size_t strpos = 0; strpos < len; strpos++) {
        width += boundsFromChar(str[strpos], settings);
    }
    return width;
}

uint16_t LedMatrix::getWidth() {
    return width;
}

uint16_t LedMatrix::getHeight() {
    return height;
}

void LedMatrix::setBlur(double blur) {
    this->blur = max(min(blur, 1.0), 0.0);
}

void LedMatrix::timeToStr(uint32_t msTime, char* hStr, char* mStr, char* sStr, char* msStr, bool oneMsDigit) {
  uint16_t ms = msTime % 1000;
  uint32_t seconds = (msTime / 1000) % 60;
  uint16_t minutes = (msTime / 60000) % 60;
  uint16_t hours = msTime / 36000000;
  if(hours > 0) {
    sprintf(hStr, "%i", hours);
  }
  if(minutes > 0) {
    sprintf(mStr, "%i", minutes);
  }
  if(seconds < 10) {
    sprintf(sStr, "0%i", seconds);
  } else {
    sprintf(sStr, "%i", seconds);
  }
  if(ms == 0) {
    sprintf(msStr, "000");
  } else if(ms < 10) {
    sprintf(msStr, "00%i", ms);
  } else if(ms < 100) {
    sprintf(msStr, "0%i", ms);
  } else {
    sprintf(msStr, "%i", ms);
  }
  if(oneMsDigit) {
    msStr[1] = 0;
  }
}

void LedMatrix::printTime(int x, int y, uint32_t ms, bool oneMsDigit) {
  char hStr[10] = "\0";
  char mStr[3]  = "\0";
  char sStr[3];
  char msStr[4];
  x+=2;
  timeToStr(ms, hStr, mStr, sStr, msStr, oneMsDigit);
  if(*hStr) {
    x = print(hStr, x - 2, y + 1, 0xFFFF00, FONT_SIZE_SMALL + MONOSPACE + 1);
    x = print(":", x- 1, y, 0xAA0000);
  }
  if(*mStr) {
    x = print(mStr, x - 2, y + 1, 0x00FFFF, FONT_SIZE_SMALL + MONOSPACE + 1);
    x = print(":", x - 1, y, 0xAA0000);
  }
  x = print(sStr, x - 1, y, 0x00FF00, 1);
  x = print(".", x- 1, y, 0xAA0000, 0);
  x = print(msStr[0], x, y, 0xFF00FF, 1);
  x = print(msStr + 1, x, y + 3, 0xFF00FF, FONT_SIZE_SMALL + 1);
}