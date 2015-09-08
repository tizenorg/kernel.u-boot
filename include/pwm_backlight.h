/*
 * Header file for PWM BACKLIGHT driver.
 *
 * Copyright (c) 2011 Samsung Electronics
 * Donghwa Lee <dh09.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _PWM_BACKLIGHT_H_
#define _PWM_BACKLIGHT_H_

struct pwm_backlight_data {
	int pwm_id;
	int period;
	int max_brightness;
	int brightness;
};

extern int pwm_backlight_init(struct pwm_backlight_data *pd);

#endif /* _PWM_BACKLIGHT_H_ */
