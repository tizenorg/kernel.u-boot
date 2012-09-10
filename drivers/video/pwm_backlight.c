/*
 * PWM BACKLIGHT driver for Board based on S5PC210. 
 *
 * Author: Donghwa Lee  <dh09.lee@samsung.com>
 *
 * Derived from linux/drivers/video/backlight/pwm_backlight.c
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <common.h>
#include <pwm.h>
#include <pwm_backlight.h>
#include <linux/types.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/gpio.h>
#include <asm/arch/pwm.h>

static struct pwm_backlight_data *pwm;

static int pwm_backlight_update_status(void)
{
	int brightness = pwm->brightness;
	int max = pwm->max_brightness;

	if (brightness == 0) {
		pwm_config(pwm->pwm_id, 0, pwm->period);
		pwm_disable(pwm->pwm_id);
	} else {
		pwm_config(pwm->pwm_id,
			brightness * pwm->period / max, pwm->period);
		pwm_enable(pwm->pwm_id);
	}
	return 0;
}

int pwm_backlight_init(struct pwm_backlight_data *pd)
{
	pwm = pd;

	pwm_backlight_update_status();

	return 0;
}

static int do_pwm_backlight(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc == 1) {
		cmd_usage(cmdtp);
		return 1;
	}

	if (argc > 3) {
		cmd_usage(cmdtp);
		return 1;
	}

	if (strcmp(argv[1], "brightness") == 0) {
		unsigned int brightness = simple_strtoul(argv[2], NULL, 10);

		printf("backlight brightness: %d\n", brightness);
		pwm->brightness = brightness
		    ;
		if (brightness > 100)
			pwm->brightness = 100;
		else if (brightness < 0)
			pwm->brightness = 0;

		pwm_backlight_update_status();
	} else if (strcmp(argv[1], "status") == 0)
		printf("current backlight brightness: %d\n", pwm->brightness);

	return 1;
}

U_BOOT_CMD(
	backlight,		3,	1, do_pwm_backlight,
	"brightness control",
	"status - print current backlight brightness\n"
	"brightness value(1~100), 0: Turn off the backlight\n"
);
