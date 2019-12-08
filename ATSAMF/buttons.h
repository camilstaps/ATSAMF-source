/* Copyright (C) 2020 Camil Staps <pa5et@camilstaps.nl> */

#ifndef _H_BUTTONS
#define _H_BUTTONS

#include "ATSAMF.h"

struct inputs {
  union {
    unsigned char port;
    struct {
      unsigned char encoder_data:1;
      unsigned char encoder_clock:1;
      unsigned char _unused:1;
      unsigned char encoder_button:1;
      unsigned char rit:1;
      unsigned char keyer:1;
    };
  };

  union {
    struct {
      unsigned char up:1;
      unsigned char down:1;
    };
    unsigned char encoder_value:2;
  };
  unsigned char encoder_last_clock:1;
};

void buttons_isr(void);
unsigned int time_rit(void);
unsigned int time_keyer(void);

#endif

// vim: tabstop=2 shiftwidth=2 expandtab:
