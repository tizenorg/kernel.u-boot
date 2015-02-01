/*
 * (C) Copyright 2010 Samsung Electronics
 * Donghwa Lee <mk7.kang@samsung.com>
 */

#define INFO_ACTION_MAGIC	0xcafe

/* use INFORM4~5 registers */
struct info_action {
	u32 magic;
	u32 action;
};

enum {
	INFO_ACTION_RESERVED,		/* Since action should be start 1 */
	INFO_ACTION_SDCARD_BOOT,
	INFO_ACTION_LCD_CONSOLE,
	INFO_ACTION_UMS,
};

void info_action_check(void);
