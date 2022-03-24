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

#ifndef _H_KEY
#define _H_KEY

#define KEY_IAMBIC 0
#define KEY_STRAIGHT 1

#ifdef __cplusplus
extern "C"{
#endif

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

extern void straight_key_handle_enable(void);
extern void straight_key_handle_disable(void);

extern void key_handle_start(void);
extern void key_handle_end(void);
extern void key_handle_dash(void);
extern void key_handle_dashdot_end(void);
extern void key_handle_dot(void);

#ifdef __cplusplus
}
#endif

#endif

// vim: tabstop=2 shiftwidth=2 expandtab:
