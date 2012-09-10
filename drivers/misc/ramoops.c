/*
 * (C) Copyright 2010 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 */
#include <common.h>
#include <ramoops.h>
#ifdef CONFIG_S5PC1XXFB
#include <fbutils.h>
#endif

static char msg[RAMOOPS_SIZE];
static char raw[RAMOOPS_SIZE];
static int dumped;

static void ramoops_show(void)
{
	int i;
#ifdef CONFIG_S5PC1XXFB
	unsigned int chars;
#endif

	if (!dumped) {
		puts("No Messages\n");
		return;
	}

#ifdef CONFIG_S5PC1XXFB
	init_font();
	fb_clear(0);
	set_font_color(FONT_WHITE);
	chars = get_chars();
	printf("chars %d\n", chars);
#endif

	printf("\n\n");
	for (i = 0; i < RAMOOPS_SIZE; i++) {
		char s[1];
		sprintf(s, "%c", msg[i]);
		puts(s);
#ifdef CONFIG_S5PC1XXFB
		if (!s5p_no_lcd_support() && i > (RAMOOPS_SIZE - chars))
			fb_printf(s);
#endif
	}
	printf("\n\n");

#ifdef CONFIG_S5PC1XXFB
	exit_font();
#endif
}

static void ramoops_parser(char *raw)
{
	int i;
	int start = 0;
	int offset = 0;

	for (i = 0; i < RAMOOPS_SIZE; i++) {
		if (raw[i] == '[' && raw[i + 13] == ']') {
			int size = i - 3 - start;

			memcpy(msg + offset, raw + start, size);

			offset += size;
			start = i + 15;
			i += 15;
		}
	}

	memcpy(msg + offset, raw + start, RAMOOPS_SIZE - start);
}

int ramoops_init(unsigned int base)
{
	unsigned int *msg_header = (unsigned int *)base;

	dumped = 0;

	if (*msg_header != RAMOOPS_HEADER)
		return -1;

	/*
	 * 1. Display the text at serial fully
	 * 2. Display the required text at LCD since size limitation
	 * Remove the Stack
	 */

	memcpy(raw, (void *)base, RAMOOPS_SIZE);
	memset((void *)base, 0x0, RAMOOPS_SIZE);

	ramoops_parser(raw);

	dumped = 1;

	ramoops_show();

	return 0;
}

static int do_ramoops(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	switch (argc) {
	case 2:
		if (strncmp(argv[1], "show", 2) == 0) {
			ramoops_show();
		} else {
			cmd_usage(cmdtp);
			return 1;
		}
		break;
	default:
		cmd_usage(cmdtp);
		return 1;
	}

	return 0;
}

U_BOOT_CMD(
	ramoops,		CONFIG_SYS_MAXARGS,	1, do_ramoops,
	"RAM Oops/Panic Logger",
	"show - show messages\n"
);
