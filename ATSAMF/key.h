/* Copyright (C) 2020 Camil Staps <pa5et@camilstaps.nl> */

#ifndef _H_KEY
#define _H_KEY

#include "ATSAMF.h"

#define KEY_IAMBIC 0
#define KEY_STRAIGHT 1

struct key_state {
  unsigned char mode:1;
  unsigned char timeout:1;
  unsigned char dot:1;
  unsigned char dash:1;
  unsigned char speed:5;
  unsigned char dot_time;
  unsigned int dash_time;
  unsigned int timer;
};

void adjust_cs(byte);
void load_cw_speed(void);
byte key_active(void);
void straight_key(void);
void key_isr(void);
void iambic_key(void);

#endif

// vim: tabstop=2 shiftwidth=2 expandtab:
