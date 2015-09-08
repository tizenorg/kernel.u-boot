/*
 * (C) Copyright 2010 Samsung Electronics
 * Donghwa Lee <dh09.lee@samsung.com>
 */
#include <common.h>
#include <info_action.h>
#include <mmc.h>
#ifdef CONFIG_S5PC1XXFB
#include <fbutils.h>
extern int s5p_no_lcd_support(void);
#endif

static int __micro_usb_attached(void)
{
	return 0;
}

int micro_usb_attached(void) __attribute__((weak, alias("__micro_usb_attached")));

void info_action_check(void)
{
	struct info_action *ia = (struct info_action *) CONFIG_INFO_ADDRESS;
	char buf[64];
	unsigned int count = 0;
	struct mmc *mmc;

	if (ia->magic != INFO_ACTION_MAGIC)
		goto done;

	switch (ia->action) {
	case INFO_ACTION_SDCARD_BOOT:
		puts("sdcard boot\n");

		/*
		 * We need to initialize SD card for SD-boot.
		 * SD card device index : 2
		 */
		mmc = find_mmc_device(2);
		if (!mmc) {
			printf("Not found SD-card..insert SD-card!!\n");
			goto done;
		}
		mmc_init(mmc);

		/* Please add the parameters and just 'run mmcboot' */
		count += sprintf(buf + count, "run mmcboot; ");
		setenv("bootcmd", buf);
		setenv("mmcdev", "2");
		setenv("mmcrootpart", "0");
		setenv("mmcbootpart", "1");
		break;
	case INFO_ACTION_LCD_CONSOLE:
		printf("lcd console mode\n");
		break;
	case INFO_ACTION_UMS:
		puts("ums mode....\n");
#ifdef CONFIG_S5PC1XXFB
		if (!s5p_no_lcd_support()) {
			init_font();
			set_font_color(FONT_WHITE);
			fb_printf("USB Mass Storage (UMS) Mode...\n");
		}
#endif
		if (!micro_usb_attached()) {
			puts("USB cable not attached !!!");
			break;
		}

		run_command("ums 0:6", 0);
		break;
	default:
		break;
	}

done:
	memset(ia, 0x0, sizeof(struct info_action));
}
