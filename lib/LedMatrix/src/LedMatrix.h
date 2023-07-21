#pragma once
#include <Arduino.h>
#include <FastLED.h>

#define FONT_SMALL_WIDTH 4
#define FONT_SMALL_HEIGHT 5

#define FONT_NORMAL_WIDTH 5
#define FONT_NORMAL_HEIGHT 8

#define UNDERLINE          0b10000000
#define MONOSPACE          0b01000000
#define UPPERCASE          0b00100000
#define FONT_SIZE_SMALL    0b00010000
#define FONT_SIZE_BIG      0b00000000
#define SPACING            0b00000111

#define FONT_SETTINGS_DEFAULT UPPERCASE + 1

typedef uint16_t (*PixelConverter)(uint16_t x, uint16_t y);

class LedMatrix {
public:
    LedMatrix();
    LedMatrix(CRGB* leds, size_t numLeds, PixelConverter pixelConverter, size_t width = 32, size_t height = 8);

    void setPixel(int x, int y, CRGB color, bool additive = false);
    void renderPixel(int x, int y, CRGB color);
    /**
     * @brief Prints a char array to screen
     * 
     * @param str The string
     * @param x absolute x pos
     * @param y absolute y pos
     * @param color color of text and underline
     * @param settings font settings see WS2812B.h
     * @param maxWidth stop printing before x + maxWidth reached
     * @param offset offset text by x pixels. Doesnt affect any bounds
     * @return int xPos for next call to print() or -1 if maxWidth got exeeded
     */
    int print(const char* str, int x, int y, CRGB color, uint8_t settings = FONT_SETTINGS_DEFAULT, int maxWidth = 1000, int offset = 0);
    void printSlideOverflow(const char* str, int x, int y, int maxX, CRGB color, uint8_t settings = FONT_SETTINGS_DEFAULT);
    void printSlideOverflow(char str, int x, int y, int maxX, CRGB color, uint8_t settings);
    int print(char digit, int x, int y, CRGB color, uint8_t settings = FONT_SETTINGS_DEFAULT, uint8_t xStart = 0, uint8_t maxWidth = 10);
    void line(double x1, double y1, double x2, double y2, CRGB color);
    void line(int x1, int y1, int x2, int y2, CRGB color);
    void rect(int x1, int y1, int x2, int y2, CRGB color);
    void dot(int x, int y, CRGB color);
    void printTime(int x, int y, int32_t ms, bool oneMsDigit = false);

    static int textWidth(const char* str, uint8_t settings = FONT_SETTINGS_DEFAULT);
    static int boundsFromChar(char digit, uint8_t settings);
    static void timeToStr(int32_t msTime, char* hStr, char* mStr, char* sStr, char* msStr, bool oneMsDigit);

    uint16_t getWidth();
    uint16_t getHeight();

    void setBlur(double blur);
private:
    CRGB* leds;
    size_t numLeds;

    PixelConverter pixelConverter;

    size_t width;
    size_t height;

    double blur;
};
