/* Copyright (C) 2020 Camil Staps <pa5et@camilstaps.nl> */

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
