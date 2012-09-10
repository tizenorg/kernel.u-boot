/* linux/arm/arch/mach-s5pc110/include/mach/s6e63m0.h
 *
 * Header file for NT39411 LCD Panel driver.
 *
 * Copyright (c) 2011 Samsung Electronics
 * Donghwa Lee <dh09.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _NT39411_H_
#define _NT39411_H_

struct nt39411_platform_data {
	struct swi_platform_data swi;
	int a_onoff;
	int b_onoff;
	unsigned int brightness;
};

#define NT39411_MAX_INTENSITY		10
#define NT39411_DEFAULT_INTENSITY	7
#define MAX_LEVEL			10

#define ALL_ON				21
#define A1_A3_ONOFF			20
#define B1_B2_ONOFF			19
#define A1_A3_CURRENT			18
#define B1_B2_CURRENT			17


#define A1_A3_ALL_ON			8
#define	A1_A2_ON			7
#define	A1_A3_ON			6
#define	A1_ON				5
#define	A2_A3_ON			4
#define	A2_ON				3
#define	A3_ON				2
#define A1_A3_ALL_OFF			1

#define B1_B2_ALL_ON			4
#define B1_ON				3
#define B2_ON				2
#define B1_B2_ALL_OFF			1

extern void nt39411_set_platform_data(struct nt39411_platform_data *pd);
extern void nt39411_send_intensity(unsigned int enable);

#endif /* _NT39411_H_ */
