#pragma once
#include <Arduino.h>

#define NO_CHAR {0b11111111,0b11111111,0b11111111,0b11111111}

#define ASCII_OFFSET 33

#define FONT_SMALL_WIDTH 3
#define FONT_SMALL_HEIGHT 5

#define FONT_NORMAL_WIDTH 4
#define FONT_NORMAL_HEIGHT 8

constexpr uint8_t fontSmall[100][4] = {
    {
        0b00011101,// !
        0b00000000,
        0b00000000,
        0b00000000,
    }, {
        0b00011000,// "
        0b00000000,
        0b00011000,
        0b00000000,
    }, {
        0b00001010,// #
        0b00011111,
        0b00011111,
        0b00001010,
    }, {
        0b00010010,// $
        0b00010101,
        0b00011111,
        0b00001001,
    }, {
        0b00010011,// %
        0b00001001,
        0b00010110,
        0b00011001,
    }, {
        0b00001101,// &
        0b00010110,
        0b00011001,
        0b00000000,
    }, {
        0b00000011,// '
        0b00000000,
        0b00000000,
        0b00000000,
    }, {
        0b00010001,// (
        0b00001110,
        0b00000000,
        0b00000000,
    }, {
        0b00001110,// )
        0b00010001,
        0b00000000,
        0b00000000,
    }, {
        0b00001100,// *
        0b00001100,
        0b00000000,
        0b00000000,
    }, {
        0b00000100,// +
        0b00001110,
        0b00000100,
        0b00000000,
    }, {
        0b00000011,// ,
        0b00000000,
        0b00000000,
        0b00000000,
    }, {
        0b00000100,// -
        0b00000100,
        0b00000000,
        0b00000000,
    }, {
        0b00000001,// .
        0b00000000,
        0b00000000,
        0b00000000,
    }, {
        0b00000011,// /
        0b00000110,
        0b00011000,
        0b00000000,
    }, {
        0b00011111,// 0
        0b00010001,
        0b00011111,
        0b00000000,
    }, {
        0b00011111,// 1
        0b00000000,
        0b00000000,
        0b00000000,
    }, {
        0b00011101,// 2
        0b00010101,
        0b00010111,
        0b00000000,
    }, {
        0b00011111,// 3
        0b00010101,
        0b00010001,
        0b00000000,
    }, {
        0b00011111,// 4
        0b00000100,
        0b00011100,
        0b00000000,
    }, {
        0b00010111,// 5
        0b00010101,
        0b00011101,
        0b00000000,
    }, {
        0b00010111,// 6
        0b00010101,
        0b00011111,
        0b00000000,
    }, {
        0b00011100,// 7
        0b00010011,
        0b00010000,
        0b00000000,
    }, {
        0b00011111,// 8
        0b00010101,
        0b00011111,
        0b00000000,
    }, {
        0b00011111,// 9
        0b00010101,
        0b00011101,
        0b00000000,
    }, {
        0b00001010,// :
        0b00000000,
        0b00000000,
        0b00000000,
    }, {
        0b00001011,// ;
        0b00000000,
        0b00000000,
        0b00000000,
    }, {
        0b00010001,// <
        0b00001010,
        0b00000100,
        0b00000000,
    }, {
        0b00001010,// =
        0b00001010,
        0b00000000,
        0b00000000,
    }, {
        0b00000100,// >
        0b00001010,
        0b00010001,
        0b00000000,
    }, {
        0b00011100,// ?
        0b00010101,
        0b00010000,
        0b00000000,
    }, {
        0b00001111,// @
        0b00010101,
        0b00010101,
        0b00001010,
    }, {
        0b00001111,// A
        0b00010100,
        0b00001111,
        0b00000000,
    }, {
        0b00001110,// B
        0b00010101,
        0b00011111,
        0b00000000,
    }, {
        0b00010001,// C
        0b00010001,
        0b00001110,
        0b00000000,
    }, {
        0b00001110,// D
        0b00010001,
        0b00011111,
        0b00000000,
    }, {
        0b00010001,// E
        0b00010101,
        0b00011111,
        0b00000000,
    }, {
        0b00010000,// F
        0b00010100,
        0b00011111,
        0b00000000,
    }, {
        0b00010111,// G
        0b00010001,
        0b00001110,
        0b000000000,
    }, {
        0b00011111,// H
        0b00000100,
        0b00011111,
        0b00000000,
    }, {
        0b00011111,// I
        0b00000000,
        0b00000000,
        0b00000000,
    }, {
        0b00011111,// J
        0b00010001,
        0b00000011,
        0b00000000,
    }, {
        0b00010111,// K
        0b00001100,
        0b00000100,
        0b00011111,
    }, {
        0b00000001,// L
        0b00011111,
        0b00000000,
        0b00000000,
    }, {
        0b00011111,// M
        0b00001000,
        0b00001000,
        0b00011111,
    }, {
        0b00011111,// N
        0b00000110,
        0b00001100,
        0b00011111,
    }, {
        0b00001110,// O
        0b00010001,
        0b00001110,
        0b00000000,
    }, {
        0b00011100,// P
        0b00010100,
        0b00011111,
        0b00000000,
    }, {
        0b00000001,// Q
        0b00001111,
        0b00010001,
        0b00001110,
    }, {
        0b00001101,// R
        0b00010010,
        0b00011111,
        0b00000000,
    }, {
        0b00010111,// S
        0b00010101,
        0b00001101,
        0b00000000,
    }, {
        0b00010000,// T
        0b00011111,
        0b00010000,
        0b00000000,
    }, {
        0b00011111,// U
        0b00000001,
        0b00011110,
        0b00000000,
    }, {
        0b00011110,// V
        0b00000001,
        0b00000010,
        0b00011100,
    }, {
        0b00011111,// W
        0b00000010,
        0b00000010,
        0b00011111,
    }, {
        0b00001001,// X
        0b00000110,
        0b00001001,
        0b00000000,
    }, {
        0b00011110,// Y
        0b00000101,
        0b00011101,
        0b00000000,
    }, {
        0b00010001,// Z
        0b00011001,
        0b00010101,
        0b00010011,
    }, {
        0b00010001,// [
        0b00011111,
        0b00000000,
        0b00000000,
    }, {
        0b00011000,// backslash
        0b00000110,
        0b00000011,
        0b00000000,
    }, {
        0b00011111,// ]
        0b00010001,
        0b00000000,
        0b00000000,
    }, {
        0b00001000,// ^
        0b00010000,
        0b00001000,
        0b00000000,
    }, {
        0b00000001,// _
        0b00000001,
        0b00000001,
        0b00000001,
    }, {
        0b00000000,// '
        0b00000000,
        0b00000000,
        0b00000000,
    }, {
        0b00001111,// a
        0b00000110,
        0b00001001,
        0b00000110,
    }, {
        0b00000010,// b
        0b00000101,
        0b00011111,
        0b00000000,
    }, {
        0b00001001,// c
        0b00000110,
        0b00000000,
        0b00000000,
    }, {
        0b00011111,// d
        0b00000101,
        0b00000010,
        0b00000000,
    }, {
        0b00001100,// e
        0b00010101,
        0b00001110,
        0b00000000
    }, {
        0b00010100,// f
        0b00001111,
        0b00000000,
        0b00000000,
    }, {
        0b00011111,// g
        0b00010101,
        0b00011000,
        0b00000000,
    }, {
        0b00011111,// h
        0b00000100,
        0b00000011,
        0b00000000,
    }, {
        0b00010111,// i
        0b00000000,
        0b00000000,
        0b00000000,
    }, {
        0b00010110,// j
        0b00000001,
        0b00000000,
        0b00000000,
    }, {
        0b00000101,// k
        0b00000010,
        0b00011111,
        0b00000000,
    }, {
        0b00000001,
        0b00011110,// l
        0b00000000,
        0b00000000,
    }, {
        0b00000011,// m
        0b00000100,
        0b00000100,
        0b00001111,
    }, {
        0b00000011,// n
        0b00000100,
        0b00000111,
        0b00000000,
    }, {
        0b00000110,// o
        0b00001001,
        0b00000110,
        0b00000000,
    }, {
        0b00001000,// p
        0b00010100,
        0b00011111,
        0b00000000,
    }, {
        0b00011111,// q
        0b00010100,
        0b00001000,
        0b00000000,
    }, {
        0b00001000,// r
        0b00000111,
        0b00000000,
        0b00000000,
    }, {
        0b00001010,// s
        0b00000101,
        0b00000000,
        0b00000000,
    }, {
        0b00001001,// t
        0b00011110,
        0b00000000,
        0b00000000,
    }, {
        0b00000110,// u
        0b00000001,
        0b00000111,
        0b00000000,
    }, {
        0b00000110,// v
        0b00000001,
        0b00000110,
        0b00000000,
    }, {
        0b00000110,// w
        0b00000001,
        0b00000010,
        0b00000111,
    }, {
        0b00000101,// x
        0b00000010,
        0b00000101,
        0b00000000,
    }, {
        0b00001100,// y
        0b00000010,
        0b00001101,
        0b00000000,
    }, {
        0b00001011,// z
        0b00001111,
        0b00001101,
        0b00000000,
    }, {
        0b00010001,// {
        0b00011111,
        0b00000100,
        0b00000000,
    }, {
        0b00011011,// |
        0b00000000,
        0b00000000,
        0b00000000,
    }, {
        0b00000100,// }
        0b00011111,
        0b00010001,
        0b00000000,
    }, {
        0b00001000,// ~
        0b00000100,
        0b00001000,
        0b00000100,
    }
};

constexpr uint8_t fontNormal[70][5] = {
    {
        0b11111101,// !
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    }, {
        0b11000000,// "
        0b00000000,
        0b11000000,
        0b00000000,
        0b00000000
    }, {
        0b01000010,// #
        0b11111111,
        0b01000010,
        0b11111111,
        0b01000010
    }, NO_CHAR // $
    , {
        0b11000010,// %
        0b00100101,
        0b01011010,
        0b10100100,
        0b01000011
    }, NO_CHAR// &
    , {
        0b11000000,
        0b00000000,// '
        0b00000000,
        0b00000000,
        0b00000000
    }, {
        0b10000001,// (
        0b01111110,
        0b00000000,
        0b00000000,
        0b00000000
    }, {
        0b01111110,// )
        0b10000001,
        0b00000000,
        0b00000000,
        0b00000000
    }, {
        0b00100100,// *
        0b00011000,
        0b00100100,
        0b00000000,
        0b00000000
    }, {
        0b00010000,// +
        0b00010000,
        0b01111100,
        0b00010000,
        0b00010000
    }, {
        0b00000111,// ,
        0b00000001,
        0b00000000,
        0b00000000,
        0b00000000
    }, {
        0b00010000,// -
        0b00010000,
        0b00010000,
        0b00010000,
        0b00000000
    }, {
        0b00000011,// .
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    }, {
        0b00000011,// /
        0b00001100,
        0b00110000,
        0b11000000,
        0b00000000
    }, {
        0b11111111,// 0
        0b10000001,
        0b10000001,
        0b11111111,
        0b00000000
    }, {
        0b11111111, // 1
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    }, {
        0b11110001,// 2
        0b10010001,
        0b10010001,
        0b10011111,
        0b00000000
    }, {
        0b11111111,// 3
        0b10010001,
        0b10010001,
        0b10000001,
        0b00000000,
    }, {
        0b11111111,// 4
        0b00001000,
        0b00001000,
        0b11111000,
        0b00000000
    }, {
        0b10011111,// 5
        0b10010001,
        0b10010001,
        0b11110001,
        0b00000000,
    }, {
        0b10011111,// 6
        0b10010001,
        0b10010001,
        0b11111111,
        0b00000000,
    }, {
        0b11100000,// 7
        0b10011111,
        0b10000000,
        0b10000000,
        0b00000000
    }, {
        0b11111111,// 8
        0b10010001,
        0b10010001,
        0b11111111,
        0b00000000,
    }, {
        0b11111111,// 9
        0b10010001,
        0b10010001,
        0b11110001,
        0b00000000
    }, {
        0b01100110,// :
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    }, {
        0b01100111,// ;
        0b01100011,
        0b00000000,
        0b00000000,
        0b00000000
    }, {
        0b10000001,// <
        0b01000010,
        0b00100100,
        0b00011000,
        0b00000000
    }, {
        0b00101000,// =
        0b00101000,
        0b00101000,
        0b00000000,
        0b00000000
    }, {
        0b00011000,// >
        0b00100100,
        0b01000010,
        0b10000001,
        0b00000000
    }, {
        0b11110000,// ?
        0b10010000,
        0b10011101,
        0b10000000,
        0b00000000
    }, {
        0b01101111,// @
        0b10010010,
        0b10010001,
        0b10001111,
        0b01111111
    }, {
        0b11111111,// A
        0b10010000,
        0b10010000,
        0b10010000,
        0b01111111
    }, {
        0b01101110,// B
        0b10010001,
        0b10010001,
        0b10010001,
        0b11111111
    }, {
        0b10000001,// C
        0b10000001,
        0b10000001,
        0b10000001,
        0b01111110
    }, {
        0b01111110,// D
        0b10000001,
        0b10000001,
        0b10000001,
        0b11111111
    }, {
        0b10000001,// E
        0b10010001,
        0b10010001,
        0b10010001,
        0b11111111
    }, {
        0b10000000,// F
        0b10010000,
        0b10010000,
        0b10010000,
        0b11111111
    }, {
        0b10001111,// G
        0b10001001,
        0b10000001,
        0b10000001,
        0b01111110
    }, {
        0b11111111,// H
        0b00010000,
        0b00010000,
        0b11111111,
        0b00000000
    }, {
        0b10000001,// I
        0b11111111,
        0b10000001,
        0b00000000,
        0b00000000
    }, {
        0b11111111,// J
        0b10000001,
        0b00000001,
        0b00000011,
        0b00000000
    }, {
        0b10000011,// K
        0b01000100,
        0b00101000,
        0b00010000,
        0b11111111
    }, {
        0b00000001,// L
        0b00000001,
        0b00000001,
        0b11111111,
        0b00000000
    }, {
        0b11111111,// M
        0b01100000,
        0b00111000,
        0b01100000,
        0b11111111
    }, {
        0b11111111,// N
        0b00000110,
        0b00111000,
        0b11000000,
        0b11111111
    }, {
        0b01111110,// O
        0b10000001,
        0b10000001,
        0b10000001,
        0b01111110
    }, {
        0b01111000,// P
        0b10001000,
        0b10001000,
        0b11111111,
        0b00000000
    }, {
        0b00000001,// Q
        0b01111111,
        0b10000001,
        0b10000001,
        0b01111110
    }, {
        0b01111001,// R
        0b10001010,
        0b10001100,
        0b10001000,
        0b11111111
    }, {
        0b11011111,// S
        0b10010001,
        0b10010001,
        0b10010001,
        0b11110011
    }, {
        0b10000000,// T
        0b10000000,
        0b11111111,
        0b10000000,
        0b10000000
    }, {
        0b11111110,// U
        0b00000001,
        0b00000001,
        0b00000001,
        0b11111110
    }, {
        0b11111000,// V
        0b00000110,
        0b00000001,
        0b00000110,
        0b11111000
    }, {
        0b11111110,// W
        0b00000001,
        0b00011110,
        0b00000001,
        0b11111110,
    }, {
        0b11000011,// X
        0b00100100,
        0b00011000,
        0b00100100,
        0b11000011
    }, {
        0b11111110,// Y
        0b00010001,
        0b00010001,
        0b11100001,
        0b00000000
    }, {
        0b11100001,// Z
        0b10110001,
        0b10011001,
        0b10001101,
        0b10000111
    }, {
        0b10000001,// [
        0b11111111,
        0b00000000,
        0b00000000,
        0b00000000
    }, {
        0b11000000,// backslash
        0b00111000,
        0b00000111,
        0b00000000,
        0b00000000
    }, {
        0b11111111,// ]
        0b10000001,
        0b00000000,
        0b00000000,
        0b00000000
    }, {
        0b01000000,// ^
        0b10000000,
        0b0100000,
        0b00000000,
        0b00000000
    }, {
        0b00000001,// _
        0b00000001,
        0b00000001,
        0b00000001,
        0b00000001
    }, {
        0b11000000,// '
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    }
};