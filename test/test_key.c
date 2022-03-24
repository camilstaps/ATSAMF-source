/**
 * Copyright (C) 2021 Camil Staps <pa5et@camilstaps.nl>
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

		/* We get here when after 7 empty periods: the keying routine returns.
		 * If the input is not finished, we need to delay for one DOT_TIME so
		 * that the next character is loaded (see `delay`), and continue. */
		state.key.timer = DOT_TIME;
		delay(DOT_TIME);
	}

	return result;
}

#ifdef __cplusplus
}
#endif
