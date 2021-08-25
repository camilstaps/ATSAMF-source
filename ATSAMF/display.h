/* Copyright (C) 2020 Camil Staps <pa5et@camilstaps.nl> */

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
