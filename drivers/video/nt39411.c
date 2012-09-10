/*
 *  nt39411 Backlight Driver based on SWI Driver.
 *
 *  Copyright (c) 2011 Samsung Electronics
 *  Donghwa Lee <dh09.lee@samsung.com>
 *
 *  Based on Sharp's Corgi Backlight Driver
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#include <common.h>
#include <linux/types.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/gpio.h>
#include <swi.h>
#include <nt39411.h>


struct nt39411_platform_data *nt39411;

/* when nt39411 is turned on, it becomes max(100) soon. */
static int current_gamma_level = MAX_LEVEL;
static unsigned int gamma_table_16[17] = {0, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
static unsigned int gamma_table_10[11] = {0,16, 15, 14, 13, 11, 9, 7, 5, 3, 1};
static int first = 1;

static void nt39411_apply(int level)
{
	int i;

	/* A1-A3 current select */
	for (i = 0; i < A1_A3_CURRENT; i++)
		swi_transfer_command(&nt39411->swi, SWI_CHANGE);
	udelay(500);

	for (i = 0; i < gamma_table_10[level]; i++)
		swi_transfer_command(&nt39411->swi, SWI_CHANGE);
	udelay(500);

	/* B1-B2 current select */
	for (i = 0; i < B1_B2_CURRENT; i++)
		swi_transfer_command(&nt39411->swi, SWI_CHANGE);
	udelay(500);

	for (i = 0; i < gamma_table_10[level]; i++)
		swi_transfer_command(&nt39411->swi, SWI_CHANGE);
	udelay(500);

	if (first) {
		/* A1-A3 on/off select */
		for (i = 0; i < A1_A3_ONOFF; i++)
			swi_transfer_command(&nt39411->swi, SWI_CHANGE);
		udelay(500);

		/* A1-A3 all on */
		for (i = 0; i < nt39411->a_onoff; i++)
			swi_transfer_command(&nt39411->swi, SWI_CHANGE);
		udelay(500);

		/* B1-B2 on/off select */
		for (i = 0; i < B1_B2_ONOFF; i++)
			swi_transfer_command(&nt39411->swi, SWI_CHANGE);
		udelay(500);

		/* B1-B2 all on */
		for (i = 0; i < nt39411->b_onoff; i++)
			swi_transfer_command(&nt39411->swi, SWI_CHANGE);

		first = 0;
	}
	current_gamma_level = level;
}

static void nt39411_apply_brightness(int level)
{
	nt39411_apply(level);
}


static void nt39411_backlight_ctl(int intensity)
{
	/* gamma range is 1 to 10. */
	if (intensity < 1)
		intensity = 1;
	if (intensity > 10)
		intensity = 10;
	nt39411_apply_brightness(intensity);
}


void nt39411_send_intensity(unsigned int enable)
{
	int intensity = nt39411->brightness;

	if (enable)
		nt39411_backlight_ctl(intensity);
}

void nt39411_set_platform_data(struct nt39411_platform_data *pd)
{
	if (pd == NULL) {
		puts("pd is NULL.\n");
		return;
	}

	nt39411 = pd;
}
