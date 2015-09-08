/*
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 * Sanghee Kim <sh0130.kim@samsung.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <stdio_dev.h>
#include <mobile/logger.h>

DECLARE_GLOBAL_DATA_PTR;

#define LOGGER_MAGIC	0x626c6f67	/* blog */
#define LOGGER_BUF_SIZE	(32 << 10)
#define LOGGER_BUF_BASE	((u32)logger + sizeof(struct logger_info))

struct logger_info {
	u32 magic;
	u32 index;		/* point current buffer */
	u32 valid;		/* to check valid of buffer */
	u32 end;
};

/* don't set the variable. after relocation, that's reset */

static struct logger_info *logger = (struct logger_info *)CONFIG_SYS_BLOG_ADDR;

static void logger_put(const char *s)
{
	int len = strlen(s);
	memcpy((void *)logger->end, s, len);
	*(char *)(logger->end + len + 1) = '\0';

	logger->end += len;
}

void logger_putc(const char c)
{
	char buf[2];
	buf[0] = c;
	buf[1] = '\0';
	logger_put(buf);
}

void logger_puts(const char *s)
{
	logger_put(s);
}

int logger_show(int index)
{
	char bak;
	char *buf_base;
	char *buf_end;
	char *last_char;

	/* index has 0: current, -1: previous */
	if ((index < -1) || (index > 0))
		return 1;

	/* correct buffer index */
	index += LOGGER_MAX + logger->index;
	index %= LOGGER_MAX;

	if (!(logger->valid & (1 << index))) {
		printf("can't found previous log\n");
		printf("only show current log (e.g. 'ramdump show blog 0')\n");
		return 0;
	}

	buf_base = (char *)(LOGGER_BUF_BASE + LOGGER_BUF_SIZE * index);
	buf_end = buf_base + strlen(buf_base);

	/* disable logging */
	gd->flags &= ~GD_FLG_LOGINIT;

	printf("== blog index:%2d (base: 0x%p) ==============\n", index,
	       buf_base);

	while (buf_base < buf_end) {
		last_char = buf_base + CONFIG_SYS_PBSIZE;
		bak = *last_char;
		*last_char = '\0';	/* for using printf */
		printf("%s", buf_base);
		*last_char = bak;

		buf_base += CONFIG_SYS_PBSIZE - 1;
	}

	printf("===================================================\n");

	/* enable logging */
	gd->flags |= GD_FLG_LOGINIT;

	return 0;
}

int logger_get_buffer(int index, ulong * start, ulong * size)
{
	/* index has 0: current, -1: previous */
	if ((index < -1) || (index > 0))
		return 1;

	/* correct buffer index */
	index += LOGGER_MAX + logger->index;
	index %= LOGGER_MAX;

	if (!(logger->valid & (1 << index)))
		return 1;

	*start = (ulong) (LOGGER_BUF_BASE + LOGGER_BUF_SIZE * index);
	*size = (ulong) (strlen((char *)*start));

	return 0;
}

static void logger_move_buffer(int index, ulong * start, ulong * size)
{
	void *bak = (void *)*start;

	/* we allocate buffer in fb area */
	*start -= (CONFIG_FB_SIZE << 20);

	memcpy((void *)*start, bak, *size);
}

int logger_set_param(void)
{
	u32 start, size;
	char buf[512];
	int count = 0;

	if (logger_get_buffer(0, (ulong *) & start, (ulong *) & size))
		return 1;

	logger_move_buffer(0, (ulong *) & start, (ulong *) & size);

	count = sprintf(buf, "%s", getenv("bootargs"));
	sprintf(buf + count, " bootloader_log=%d@0x%x", size, start);
	setenv("bootargs", buf);

	return 0;
}

int logger_init_f(void)
{
	if (logger->magic == LOGGER_MAGIC) {
		logger->index = ++logger->index % LOGGER_MAX;
	} else {
		logger->magic = LOGGER_MAGIC;
		logger->index = 0;
		logger->valid = 0;
	}
	logger->valid |= (1 << logger->index);
	/* init with start addr */
	logger->end = LOGGER_BUF_BASE + LOGGER_BUF_SIZE * logger->index;

	/* enable logging */
	gd->flags |= GD_FLG_LOGINIT;

	/* add debug message after here */

	return 0;
}

#if 0
#define DEV_NAME	"logger"

int drv_logger_init(void)
{
	struct stdio_dev logger;
	int rc;

	/* Device initialization */
	memset(&logger, 0, sizeof(logger));

	strcpy(logger.name, DEV_NAME);
	logger.ext = 0;		/* No extensions */
	logger.flags = DEV_FLAGS_OUTPUT;	/* Output only */
	logger.putc = logger_putc;	/* 'putc' function */
	logger.puts = logger_puts;	/* 'puts' function */

	rc = stdio_register(&logger);

	if (rc == 0)
		setenv("stderr", DEV_NAME);	/* use the stderr for logging */

	return (rc == 0) ? 1 : rc;
}
#endif
