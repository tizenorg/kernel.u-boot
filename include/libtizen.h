/*
 * (C) Copyright 2012 Samsung Electronics
 * Donghwa Lee <dh09.lee@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _LIBTIZEN_H_
#define _LIBTIZEN_H_

#include <lcd.h>

#define HD_RESOLUTION	0

void get_tizen_logo_info(vidinfo_t *vid);
void draw_thor_progress(unsigned long long int total_file_size, unsigned long long int downloaded_file_size);
void draw_thor_init_screen(void);
void draw_thor_screen(void);
void draw_thor_fail_screen(void);
void draw_thor_cable_info(void);

#endif	/* _LIBTIZEN_H_ */
