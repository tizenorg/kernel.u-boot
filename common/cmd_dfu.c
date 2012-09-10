/*
 * Copyright (C) 2011 Samsung Electrnoics
 * Andrzej Pietrasiewicz <andrzej.p@samsung.com>
 *
 */

#include <common.h>
#include <command.h>
#include <dfu.h>
#include <flash_entity.h>

static int do_dfu(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	board_dfu_init();
	dfu_init();
	while (1)
	{
		int irq_res;
		/* Handle control-c and timeouts */
		if (ctrlc()) {
			printf("The remote end did not respond in time.\n");
			goto fail;
		}

		irq_res = usb_gadget_handle_interrupts();
	}
fail:
	dfu_cleanup();
	board_dfu_cleanup();
	return -1;
}

U_BOOT_CMD(dfu, CONFIG_SYS_MAXARGS, 1, do_dfu,
	"Use the DFU [Device Firmware Upgrade]",
        "dfu - device firmware upgrade"
);

