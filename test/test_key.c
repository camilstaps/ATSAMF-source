/* Copyright (C) 2020 Camil Staps <pa5et@camilstaps.nl> */

#include "test_key.h"
#include <Arduino.h>
#include "ATSAMF.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C"{
#endif

#define DOT_TIME 10
#define BUFFER_SIZE 128

struct atsamf state;

static char *character;
static char result[BUFFER_SIZE];
static char *result_ptr;

static void add_result(char c) {
	if (result_ptr-BUFFER_SIZE >= result_ptr) {
		printf("result buffer overflow\n");
		exit(-1);
	}
	*result_ptr++ = c;
	*result_ptr = '\0';
}

void delay(unsigned long ms) {
	for (; ms; ms--) {
		if (state.key.timer % DOT_TIME == DOT_TIME / 2) {
			if (*character)
				character++;
		}

		key_isr();
	}
}

void invalidate_display(void) {
}

int digitalRead(uint8_t pin) {
	int enabled = 0;

	if (pin == DASHin)
		enabled = *character == '-' || *character == 'B';
	else if (pin == DOTin)
		enabled = *character == '.' || *character == 'B';
	else {
		printf ("digitalRead: unknown pin %d\n",pin);
		exit(-1);
	}

	return enabled ? LOW : HIGH;
}

void straight_key_handle_enable(void) {
}

void straight_key_handle_disable(void) {
}

void key_handle_start(void) {
}

void key_handle_end(void) {
	add_result(' ');
}

void key_handle_dash(void) {
	add_result('-');
}

void key_handle_dot(void) {
	add_result('.');
}

void key_handle_dashdot_end(void) {
}

char *run_test(char *_character) {
	state.key.dash_time = DOT_TIME * 3;
	state.key.dot_time = DOT_TIME;

	character = _character;

	result_ptr = result;
	*result_ptr = '\0';

	while (*character) {
		iambic_key();

		state.key.timer = DOT_TIME;
		delay(DOT_TIME);
	}

	return result;
}

#ifdef __cplusplus
}
#endif