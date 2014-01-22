#include <common.h>
#include <thor.h>
#include <dfu.h>
#include <g_dnl.h>
#include <pit.h>
#include <usb.h>
#include <libtizen.h>

int do_usbd_down(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *s = "thor";
	int ret;
	int retry;

	if (argc < 3)
		return CMD_RET_USAGE;
retry:
	puts("TIZEN \"THOR\" Downloader\n");

	set_download_logo(0);

	retry = 0;
	/* convert pit to dfu_alt_info env */
	pit_to_dfu_alt_info();

	ret = add_dtb_to_dfu_alt_info();
	if (ret)
		return ret;

	ret = dfu_init_pit_entities(argv[1], simple_strtoul(argv[2], NULL, 10));
	if (ret)
		return ret;

	ret = board_usb_init(0, 0);
	if (ret) {
		error("USB init failed: %d", ret);
		ret = CMD_RET_FAILURE;
		goto exit;
	}

	g_dnl_register(s);

	ret = thor_init();
	if (ret) {
		error("THOR DOWNLOAD failed: %d", ret);
		ret = CMD_RET_FAILURE;
		/* retry usbdown */
		retry = 1;
		goto exit;
	}
	/* set pit update support */
	thor_set_pit_support(PIT_SUPPORT_NORMAL);

	ret = thor_handle();
	if (ret) {
		error("THOR failed: %d", ret);
		ret = CMD_RET_FAILURE;
		/* retry usbdown */
		retry = 1;
		goto exit;
	}

exit:
	thor_set_pit_support(PIT_SUPPORT_NO);

	g_dnl_unregister();
	dfu_free_entities();

	if (retry)
		goto retry;

	set_download_logo(1);

	return 0;
}

U_BOOT_CMD(usbdown, CONFIG_SYS_MAXARGS, 1, do_usbd_down,
	   "USB Downloader for SLP",
	   "<interface> <dev>\n"
	   "  - download binary images on a device <dev>\n"
	   "    attached to interface <interface>"
);
