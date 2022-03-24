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

#ifndef _H_BUTTONS
#define _H_BUTTONS

#include "ATSAMF.h"

struct inputs {
  union {
    unsigned char port;
    struct {
      unsigned char encoder_data:1;
      unsigned char encoder_clock:1;
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
