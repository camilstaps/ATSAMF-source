/**
 * Copyright (C) 2020-2021 Camil Staps <pa5et@camilstaps.nl>
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

#ifndef _H_DISPLAY
#define _H_DISPLAY

#include "ATSAMF.h"

#define BLINK_NONE  0
#define BLINK_10HZ  (1 << 4)
#define BLINK_100HZ (1 << 2)
#define BLINK_1KHZ  (1 << 1)

#ifdef __cplusplus
extern "C"{
#endif

struct display {
  char line_1[17];
  char line_2[17];
  unsigned short blinking_1;
};

void display_init(void);
void display_isr(void);
void display_flash_circle(uint8_t);
void display_feedback(const char*);
void display_question(const char*);
void display_progress(short,short,short);
void display_clear_progress(void);
void display_delay(short);

void invalidate_display(void);

#ifdef __cplusplus
}
#endif

#endif

// vim: tabstop=2 shiftwidth=2 expandtab:
