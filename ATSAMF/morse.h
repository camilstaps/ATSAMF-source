/**
 * Copyright (C) 2020 Camil Staps <pa5et@camilstaps.nl>
 *
 * This is software for the ATSAMF rig. Fore more information, see the
 * README.md file.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 */

#ifndef _H_MORSE
#define _H_MORSE

#include "ATSAMF.h"

#define MA 0b101    // A
#define MB 0b11000  // B
#define MC 0b11010  // C
#define MD 0b1100   // D
#define ME 0b10     // E
#define MF 0b10010  // F
#define MG 0b1110   // G
#define MH 0b10000  // H
#define MI 0b100    // I
#define MJ 0b10111  // J
#define MK 0b1101   // K
#define ML 0b10100  // L
#define MM 0b111    // M
#define MN 0b110    // N
#define MO 0b1111   // O
#define MP 0b10110  // P
#define MQ 0b11101  // Q
#define MR 0b1010   // R
#define MS 0b1000   // S
#define MT 0b11     // T
#define MU 0b1001   // U
#define MV 0b10001  // V
#define MW 0b1011   // W
#define MX 0b11001  // X
#define MY 0b11011  // Y
#define MZ 0b11100  // Z

#define M0 0b111111 // 0
#define M1 0b101111 // 1
#define M2 0b100111 // 2
#define M3 0b100011 // 3
#define M4 0b100001 // 4
#define M5 0b100000 // 5
#define M6 0b110000 // 6
#define M7 0b111000 // 7
#define M8 0b111100 // 8
#define M9 0b111110 // 9

#define Mquestion 0b1001100 // ?

extern const byte MORSE_DIGITS[];

#endif

// vim: tabstop=2 shiftwidth=2 expandtab:
