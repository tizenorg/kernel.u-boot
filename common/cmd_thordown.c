/*
 * cmd_thordown.c -- USB TIZEN "THOR" Downloader gadget
 *
 * Copyright (C) 2013 Lukasz Majewski <l.majewski@samsung.com>
 * All rights reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <thor.h>
#include <dfu.h>
#include <g_dnl.h>
#include <usb.h>
#include <libtizen.h>
#include <samsung/pit.h>

int do_thor_down(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc < 4)
		return CMD_RET_USAGE;

	char *usb_controller = argv[1];
	char *interface = argv[2];
	char *devstring = argv[3];
	int dev = simple_strtoul(devstring, NULL, 10);

	const char *s = "thor";
	int ret;

	puts("TIZEN \"THOR\" Downloader\n");

#ifdef CONFIG_CMD_PIT
	/* convert pit to dfu_alt_info env */
	pit_to_dfu_alt_info();

	ret = dfu_init_pit_entities(interface, dev);
#else
	ret = dfu_init_env_entities(interface, dev);
#endif
	if (ret)
		return ret;

	int controller_index = simple_strtoul(usb_controller, NULL, 0);
	ret = board_usb_init(controller_index, USB_INIT_DEVICE);
	if (ret) {
		error("USB init failed: %d", ret);
		ret = CMD_RET_FAILURE;
		goto exit;
	}

	g_dnl_register(s);

	ret = thor_init();
	if (ret) {
		error("THOR DOWNLOAD failed: %d", ret);
		if (ret == -EINTR)
			ret = CMD_RET_SUCCESS;
		else
			ret = CMD_RET_FAILURE;

		goto exit;
	}

#ifdef CONFIG_CMD_PIT
	/* set pit update support */
	thor_set_pit_support(PIT_SUPPORT_NORMAL);
#endif

	ret = thor_handle();
	if (ret) {
		error("THOR failed: %d", ret);
		ret = CMD_RET_FAILURE;
		goto exit;
	}

exit:
#ifdef CONFIG_CMD_PIT
	thor_set_pit_support(PIT_SUPPORT_NO);
#endif
	g_dnl_unregister();
	dfu_free_entities();

#ifdef CONFIG_TIZEN
	if (ret != CMD_RET_SUCCESS)
		draw_thor_fail_screen();
	else
		lcd_clear();
#endif
	return ret;
}

U_BOOT_CMD(thordown, CONFIG_SYS_MAXARGS, 1, do_thor_down,
	   "TIZEN \"THOR\" downloader",
	   "<USB_controller> <interface> <dev>\n"
	   "  - device software upgrade via LTHOR TIZEN dowload\n"
	   "    program via <USB_controller> on device <dev>,\n"
	   "	attached to interface <interface>\n"
);
