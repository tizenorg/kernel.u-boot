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
#include <asm/arch/power.h>
#include <mobile/misc.h>
#ifdef CONFIG_LCD
#include <fbutils.h>
#endif
#ifdef CONFIG_CMD_PIT
#include <mobile/pit.h>
#endif
#ifdef CONFIG_LOGGER
#include <mobile/logger.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

#define RD_RAM			(1 << 0)
#define RD_KLOG			(1 << 1)
#define RD_DLOG			(1 << 2)
#define RD_BLOG			(1 << 3)
#define RD_FB			(1 << 4)
#define RD_ALL			(RD_RAM | RD_KLOG | RD_DLOG | RD_BLOG | RD_FB)

/*
 * GETLOG
 */

#define GLOG_MARK		(('G' << 24) | ('L' << 16))
#define GLOG_DLOG_MARK		(('D' << 24) | ('L' << 16))
#define GLOG_MARK_MASK		(0xffff << 16)

#define GLOG_MAGIC		(('G' << 24) | ('L' << 16) | ('O' << 8) | ('G'))

struct glog_header {
	u32 magic;
	u32 version;
	u32 len;
};

#define GLOG_MEMINFO_MAGIC	(GLOG_MARK | (1 << 0))
#if defined(CONFIG_S5PC110)
#define GLOG_MEM_CNT		4	/* c110 use 4 bank */
#elif defined(CONFIG_EXYNOS4210)
#define GLOG_MEM_CNT		5	/* 4210 use 5 bank */
#elif defined(CONFIG_EXYNOS4)
#define GLOG_MEM_CNT		4	/* exynos4 use 4 bank */
#else
#define GLOG_MEM_CNT		4	/* others use 4 bank */
#endif

struct glog_meminfo {
	u32 type;
	u32 len;
	u32 num;
	struct mem {
		u32 index;
		u32 start;
		u32 size;
	} s[GLOG_MEM_CNT];
};

#define GLOG_KLOGINFO_MAGIC	(GLOG_MARK | (1 << 1))
#define GLOG_KLOG_CNT		1

struct glog_kloginfo {
	u32 type;
	u32 len;
	u32 num;
	struct kloginfo {
		u32 index;
		u32 type;
		void *start;
		u32 size;
	} s[GLOG_KLOG_CNT];
};

#define GLOG_DLOGINFO_MAGIC	(GLOG_MARK | (1 << 2))
#define GLOG_DLOG_CNT		4

#define GLOG_DLOG_SYSTEM	(GLOG_DLOG_MARK | (1 << 0))
#define GLOG_DLOG_RADIO		(GLOG_DLOG_MARK | (1 << 1))
#define GLOG_DLOG_MAIN		(GLOG_DLOG_MARK | (1 << 2))
#define GLOG_DLOG_EVENT		(GLOG_DLOG_MARK | (1 << 3))

struct glog_dloginfo {
	u32 type;
	u32 len;
	u32 num;
	struct dloginfo {
		u32 index;
		u32 type;
		void *start;
		u32 size;
	} s[GLOG_DLOG_CNT];
};

#define GLOG_FBINFO_MAGIC	(GLOG_MARK | (1 << 3))
#define GLOG_FB_CNT		2

struct glog_fbinfo {
	u32 type;
	u32 len;
	u32 num;
	struct fbinfo {
		u32 index;
		void *start;
		u32 x_res;
		u32 y_res;
		u32 bpp;
	} s[GLOG_FB_CNT];
};

struct glog {
	struct glog_header hdr;
	struct glog_meminfo mem;
	struct glog_kloginfo klog;
	struct glog_dloginfo dlog;
	struct glog_fbinfo fb;
};

/*
 * RDX (Ram Dump eXtractor)
 */

#define RDX_APP_VERSION		"v5.4"
#define RDX_MEM_MAX_NUM		16

struct rdx_mem_info_s {
	int valid;
	char name[16];
	u32 start;
	u32 end;
};

struct rdx_info_s {
	char model_name[16];
	struct rdx_mem_info_s mem_info[RDX_MEM_MAX_NUM];
};

extern struct pitpart_data pitparts[];
static struct rdx_info_s rdx_info;
static int rdx_info_cnt;

static int ramdump_upload(char target);
static void ramdump_clean(void);
static inline void print_msg(char *msg)
{
#ifdef CONFIG_LCD
	fb_printf(msg);
#endif
	printf(msg);
}

static u32 virtual_to_physical(void *virtual_addr)
{
	return ((u32) virtual_addr - (0xC0000000 - CONFIG_SYS_SDRAM_BASE));
}

static int write_file(int bank, int dev, char *file, ulong start, ulong len)
{
	char cmd[128];
	char msg[64];

	sprintf(msg, "\nSave as %s\n", file);
	print_msg(msg);

	if (bank >= 0) {
		sprintf(msg, "- bank #%d: %08lx %08lx..", bank, start, len);
		print_msg(msg);
	}

	sprintf(cmd, "fatwrite mmc %d:%d 0x%lx %s 0x%lx",
		CONFIG_MMC_DEFAULT_DEV, dev, start, file, len);
	if (run_command(cmd, 0) < 0) {
		print_msg("failed\n\nplz check free space\n");
		return 1;
	}

	print_msg("ok\n");
	return 0;
}

/* show dump mode on LCD */
static int ramdump_logo(void)
{
	char *s = getenv("bootmode");
	char msg[64];

#ifdef CONFIG_LCD
	if (board_no_lcd_support())
		return 0;

	lcd_display_clear();
	init_font();
	set_font_color(FONT_WHITE);
#endif
	print_msg("Dump mode is ");
	set_font_color(FONT_RED);
	sprintf(msg, "%s", s);
	print_msg(msg);
#ifdef CONFIG_LCD
	set_font_color(FONT_WHITE);
#endif
	sprintf(msg, " - RSTST: 0x%x", readl(S5P_RST_STAT));
	print_msg(msg);
	print_msg("\n\nRAM will be saved on UMS or uploaded to HOST\n");

	return 0;
}

/* save whole RAM data as file on UMS */
static int ramdump_save(u32 target)
{
	int i;
	int dev;
	ulong start, size, check;
	char file[64];
	char *s = getenv("bootmode");
	struct glog *glog = (struct glog *)CONFIG_SYS_GETLOG_ADDR;

	printf("Save Start (target: 0x%x)\n", target);

#ifdef CONFIG_CMD_PIT
	i = get_pitpart_id(PARTS_UMS);
	dev = pitparts[i].id;
	dev = pit_adjust_id_on_mmc(dev);
#else
	/* default use mmcblk0p8 as UMS */
	dev = 8;
#endif

	if (!strncmp(s, "normal", 6))
		ramdump_logo();

	set_font_color(FONT_YELLOW);
	print_msg("\n[LOG EXTRACT]\n");
	set_font_color(FONT_WHITE);

	if (glog->hdr.magic != GLOG_MAGIC) {
		print_msg("error: invalid getlog magic\n");
		goto save_error;
	}

	/* log: klog, dlog(slog, rlog, mlog, elog), blog */

	if (!(target & RD_KLOG))
		goto save_dlog;

	if (glog->klog.type == GLOG_KLOGINFO_MAGIC) {
		char c = 'k';

		start = (ulong) virtual_to_physical(glog->klog.s[0].start);
		size = (ulong) glog->klog.s[0].size;

		sprintf(file, "mem%s_%clog_0x%08lx--0x%08lx.bin",
			s, c, start, start + size);
		if (write_file(-1, dev, file, start, size))
			goto save_error;
	} else {
		print_msg("error: invalid klog magic\n");
	}

save_dlog:
	if (!(target & RD_DLOG))
		goto save_blog;

	if (glog->dlog.type != GLOG_DLOGINFO_MAGIC) {
		print_msg("error: invalid dlog magic\n");
		goto save_blog;
	}

	if (glog->dlog.num > GLOG_DLOG_CNT) {
		print_msg("warning: dlog count exceed the max\n");
		glog->dlog.num = GLOG_DLOG_CNT;
	}

	for (i = 0; i < glog->dlog.num; i++) {
		char type[GLOG_DLOG_CNT] = { 's', 'r', 'm', 'e' };
		char c;

		if ((glog->dlog.s[i].type & GLOG_MARK_MASK) !=
		    GLOG_DLOG_MARK) {
			print_msg("error: invalid magic in dlog\n");
			continue;
		}

		start =
		    (ulong) virtual_to_physical(glog->dlog.s[i].start);
		size = (ulong) glog->dlog.s[i].size;
		c = type[glog->dlog.s[i].index & 0xff];

		sprintf(file, "mem%s_%clog_0x%08lx--0x%08lx.bin",
			s, c, start, start + size);
		if (write_file(-1, dev, file, start, size))
			goto save_error;
	}

save_blog:
#ifdef CONFIG_LOGGER
	/* log: blog */
	if (!(target & RD_BLOG))
		goto save_fb;

	for (i = 0; i < LOGGER_MAX; i++) {
		if (!logger_get_buffer(i * (-1), &start, &size)) {
			sprintf(file, "mem%s_%clog%d_0x%08lx--0x%08lx.bin",
				s, 'b', i, start, start + size);
			if (write_file(-1, dev, file, start, size))
				goto save_error;
		}
	}
#endif

save_fb:
	/* frame buffer */
	if (!(target & RD_FB))
		goto save_ram;

	set_font_color(FONT_YELLOW);
	print_msg("\n[FB SAVE]\n");
	set_font_color(FONT_WHITE);

	if (glog->fb.type == GLOG_FBINFO_MAGIC) {
		if (glog->fb.num > GLOG_FB_CNT) {
			print_msg("warning: fb count exceed the max\n");
			glog->fb.num = GLOG_FB_CNT;
		}

		for (i = 0; i < glog->fb.num; i++) {
			start = (ulong) glog->fb.s[i].start;
			size = (ulong) (glog->fb.s[i].x_res *
					glog->fb.s[i].y_res *
					(glog->fb.s[i].bpp >> 3));

			sprintf(file, "mem%s_fb%d_0x%08lx--0x%08lx.bin",
				s, i, start, start + size);
			if (write_file(-1, dev, file, start, size))
				goto save_error;
		}
	} else {
		print_msg("error: invalid fb info magic\n");
	}

save_ram:
#if !defined(CONFIG_EXYNOS4210)
	/* ram dump */
	if (!(target & RD_RAM))
		goto save_done;

	set_font_color(FONT_YELLOW);
	print_msg("\n[RAM SAVE]\n");
	set_font_color(FONT_WHITE);

	if (glog->mem.type == GLOG_MEMINFO_MAGIC) {
		if (glog->mem.num > GLOG_MEM_CNT) {
			print_msg("warning: mem bank count exceed the max\n");
			glog->mem.num = GLOG_MEM_CNT;
		}

		for (i = 0; i < glog->mem.num; i++) {
			start = glog->mem.s[i].start;
			size = glog->mem.s[i].size;
			check = min(size, 256 << 20);

			sprintf(file, "mem%s_bank%d_0x%08lx--0x%08lx.bin",
				s, i, start, start + check);
			if (write_file(i, dev, file, start, check))
				goto save_error;

			size -= check;
			if (size) {
				start += +256 << 20;

				sprintf(file,
					"mem%s_bank%d_0x%08lx--0x%08lx.bin", s,
					i + 1, start, start + size);
				if (write_file(i, dev, file, start, size))
					goto save_error;
			}
		}
	} else {
		print_msg("error: invalid mem info magic\n");
	}
#endif

save_done:
	print_msg("\nDump Done\n");
	return 0;
save_error:
	print_msg("\nDump End with ERROR\n");
	return 1;
save_reboot:
	run_command("reset", 0);
}

static int rdx_info_init(void)
{
	int i;
	char buf[16];
	char *model = getenv("model");

	printf("\nSet RDX information\n");

	if (model == NULL)
		model = "unknown";

	strcpy(rdx_info.model_name, model);
	rdx_info_cnt = 0;
}

static void rdx_info_fill(char *name, u32 start, u32 size)
{
	strcpy(rdx_info.mem_info[rdx_info_cnt].name, name);
	rdx_info.mem_info[rdx_info_cnt].valid = 1;
	rdx_info.mem_info[rdx_info_cnt].start = start;
	rdx_info.mem_info[rdx_info_cnt].end = start + size - 1;

	rdx_info_cnt++;
}

int rdx_info_get(void **info_buf, int *info_size)
{
	int i = 0;
	int size;

	size = sizeof(rdx_info.model_name);
	printf("rdx:\tmodel(%s)\n", rdx_info.model_name);
	while ((rdx_info.mem_info[i].valid) && (i < RDX_MEM_MAX_NUM)) {
		size += sizeof(struct rdx_mem_info_s);

		printf("rdx:\tstart(0x%08x) end(0x%x) name(%s)\n",
			rdx_info.mem_info[i].start,
			rdx_info.mem_info[i].end,
			rdx_info.mem_info[i].name);
		i++;
	}

	*info_buf = &rdx_info;
	*info_size = size;

	return 0;
}

/* upload LOG and RAM data to the host */
static int ramdump_upload(char target)
{
	int i;
	ulong start, size, check;
	char buf[64];
	char *s = getenv("bootmode");
	struct glog *glog = (struct glog *)CONFIG_SYS_GETLOG_ADDR;

	printf("Upload Start (target: 0x%x)\n", target);

	set_font_color(FONT_YELLOW);
	sprintf(buf, "\nRam Dump eXtractor %s\n\n", RDX_APP_VERSION);
	print_msg(buf);
	set_font_color(FONT_WHITE);

	rdx_info_init();

	if (glog->hdr.magic != GLOG_MAGIC) {
		printf("error: invalid getlog magic\n");
		goto save_ram;
	}

	/* workaround: rdx v5.4 don't support log upload yet */
	goto save_ram;

	/* log: klog, dlog(slog, rlog, mlog, elog), blog */

	if (!(target & RD_KLOG))
		goto save_dlog;

	if (glog->klog.type == GLOG_KLOGINFO_MAGIC) {
		start = (ulong) virtual_to_physical(glog->klog.s[0].start);
		size = (ulong) glog->klog.s[0].size;

		sprintf(buf, "mem%s-klog", s);
		rdx_info_fill(buf, start, size);
	} else {
		printf("error: invalid klog magic\n");
	}

save_dlog:
	if (!(target & RD_DLOG))
		goto save_blog;

	if (glog->dlog.type != GLOG_DLOGINFO_MAGIC) {
		printf("error: invalid dlog magic\n");
		goto save_blog;
	}

	if (glog->dlog.num > GLOG_DLOG_CNT) {
		printf("warning: dlog count exceed the max\n");
		glog->dlog.num = GLOG_DLOG_CNT;
	}

	for (i = 0; i < glog->dlog.num; i++) {
		char type[GLOG_DLOG_CNT] = { 's', 'r', 'm', 'e' };
		char c;

		if ((glog->dlog.s[i].type & GLOG_MARK_MASK) !=
		    GLOG_DLOG_MARK) {
			printf("error: invalid magic in dlog\n");
			continue;
		}

		start =
		    (ulong) virtual_to_physical(glog->dlog.s[i].start);
		size = (ulong) glog->dlog.s[i].size;
		c = type[glog->dlog.s[i].index & 0xff];

		sprintf(buf, "mem%s-%clog", s, c);
		rdx_info_fill(buf, start, size);
	}

save_blog:
#ifdef CONFIG_LOGGER
	/* log: blog */
	if (!(target & RD_BLOG))
		goto save_ram;

	for (i = 0; i < LOGGER_MAX; i++) {
		if (!logger_get_buffer(i * (-1), &start, &size)) {
			sprintf(buf, "mem%s-%clog%d", s, 'b', i);
			rdx_info_fill(buf, start, size);
		}
	}
#endif

save_ram:
	/* ram dump */
	if (!(target & RD_RAM))
		goto save_done;

	if (glog->mem.type == GLOG_MEMINFO_MAGIC) {
		if (glog->mem.num > GLOG_MEM_CNT) {
			printf("warning: mem bank count exceed the max\n");
			glog->mem.num = GLOG_MEM_CNT;
		}

		for (i = 0; i < glog->mem.num; i++) {
			start = glog->mem.s[i].start;
			size = glog->mem.s[i].size;
			check = min(size, 256 << 20);

			sprintf(buf, "mem%s-bank%d", s, i);
			rdx_info_fill(buf, start, size);

			size -= check;
			if (size) {
				start += +256 << 20;

				sprintf(buf, "mem%s-bank%d", s, i + 1);
				rdx_info_fill(buf, start, size);
			}
		}
	} else {
		for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
			sprintf(buf, "AP%s-bank%d", s, i);
			rdx_info_fill(buf,
				gd->bd->bi_dram[i].start, gd->bd->bi_dram[i].size);
		}
	}

save_done:
	run_command("usbdown upload", 0);
	return 0;
}

static void ramdump_clean(void)
{
	/* restore bootcmd */
	setenv("bootcmd", CONFIG_BOOTCOMMAND);

	/* clear magic code for reboot by power long key */
	if (strncmp(getenv("SLP_DEBUG_LEVEL"), "0", 1)) {
		struct glog *glog = (struct glog *)CONFIG_SYS_GETLOG_ADDR;
		if (glog->hdr.magic == GLOG_MAGIC) {
			glog->hdr.magic = 0;
			print_msg("\ngetlog magic cleared\n");
		}
	}
#ifdef CONFIG_LCD
	exit_font();
#endif
}

static int ramdump_start(u32 target)
{
	int ret;
	char *s = getenv("ramdump");

	if (s  == NULL) {
		printf("ramdump action isn't defined\n");
		/* FIXME: reset is correct? */
		run_command("reset", 0);
		return 0;
	}

	ramdump_logo();

	if (!strncmp(s, "save", 4)) {
		ret = ramdump_save(target);
	} else if (!strncmp(s, "upload", 4)) {
		ret = ramdump_upload(target);
	} else if (!strncmp(s, "both", 4)) {
		ret = ramdump_save(target);
		ret |= ramdump_upload(target);
	}

	ramdump_clean();

	return ret;
}

/* show header information */
static int ramdump_check(void)
{
	int i;
	struct glog *glog = (struct glog *)CONFIG_SYS_GETLOG_ADDR;

	/* header */
	printf("HDR:\tmagic:\t0x%08x", glog->hdr.magic);
	if (glog->hdr.magic != GLOG_MAGIC)
		printf("\t=>INVALID (!=0x%08x)\n", GLOG_MAGIC);
	else
		printf("\n");
	printf("HDR:\tver:\t0x%08x\n", glog->hdr.version);
	printf("\n");

	/* mem info */
	printf("MEM:\tmagic:\t0x%08x", glog->mem.type);
	if (glog->mem.type != GLOG_MEMINFO_MAGIC)
		printf("\t=>INVALID (!=0x%08x)\n", GLOG_MEMINFO_MAGIC);
	else
		printf("\n");
	printf("MEM:\tnum:\t%d (len:%d)\n", glog->mem.num, glog->mem.len);
	for (i = 0; i < glog->mem.num; i++) {
		if (i > GLOG_MEM_CNT)
			break;
		printf("\tbank%d:\t0x%08x--0x%08x\n",
		       i,
		       glog->mem.s[i].start,
		       glog->mem.s[i].start + glog->mem.s[i].size);
	}
	printf("\n");

	/* klog info */
	printf("KLOG:\tmagic:\t0x%08x", glog->klog.type);
	if (glog->klog.type != GLOG_KLOGINFO_MAGIC)
		printf("\t=>INVALID (!=0x%08x)\n", GLOG_KLOGINFO_MAGIC);
	else
		printf("\n");
	printf("KLOG:\tnum:\t%d (len:%d)\n", glog->klog.num, glog->klog.len);
	if (glog->klog.num == 0)
		glog->klog.num = 1;
	for (i = 0; i < glog->klog.num; i++) {
		if (i > GLOG_KLOG_CNT)
			break;
		printf("\ttype:\t0x%08x,\t0x%08x--0x%08x\n",
		       glog->klog.s[i].type,
		       virtual_to_physical(glog->klog.s[i].start),
		       virtual_to_physical(glog->klog.s[i].start +
					   glog->klog.s[i].size));
	}
	printf("\n");

	/* dlog info */
	printf("DLOG:\tmagic:\t0x%08x", glog->dlog.type);
	if (glog->dlog.type != GLOG_DLOGINFO_MAGIC)
		printf("\t=>INVALID (!=0x%08x)\n", GLOG_DLOGINFO_MAGIC);
	else
		printf("\n");
	printf("DLOG:\tnum:\t%d (len:%d)\n", glog->dlog.num, glog->dlog.len);
	for (i = 0; i < glog->dlog.num; i++) {
		if (i > GLOG_DLOG_CNT)
			break;
		printf("\ttype:\t0x%08x,\t0x%08x--0x%08x\n",
		       glog->dlog.s[i].type,
		       virtual_to_physical(glog->dlog.s[i].start),
		       virtual_to_physical(glog->dlog.s[i].start +
					   glog->dlog.s[i].size));
	}
	printf("\n");

	/* fb info */
	printf("FB:\tmagic:\t0x%08x", glog->fb.type);
	if (glog->fb.type != GLOG_FBINFO_MAGIC)
		printf("\t=>INVALID (!=0x%08x)\n", GLOG_FBINFO_MAGIC);
	else
		printf("\n");
	printf("FB:\tnum:\t%d (len:%d)\n", glog->fb.num, glog->fb.len);
	for (i = 0; i < glog->fb.num; i++) {
		if (i > GLOG_FB_CNT)
			break;
		printf("\tfb%d:\t0x%08x--0x%08x\n",
		       i,
		       (u32) glog->fb.s[i].start,
		       (u32) glog->fb.s[i].start +
		       glog->fb.s[i].x_res * glog->fb.s[i].y_res *
		       (glog->fb.s[i].bpp >> 3));
	}

	return 0;
}

/* print rb dump area */
static int ramdump_show(u32 target, int index)
{
	switch (target) {
	case RD_KLOG:		/* dump kernel log */
		break;
	case RD_DLOG:		/* dump dlog (slog, rlog, mlog, elog) */
		break;
#ifdef CONFIG_LOGGER
	case RD_BLOG:		/* dump bootloader log */
		return logger_show(index);
#endif
	}

	return 0;
}

static int do_ramdump(cmd_tbl_t * cmdtp, int flag, int argc, char *const argv[])
{
	int ret;
	int index = -1;
	u32 target = RD_ALL;

	if (argc < 2)
		return cmd_usage(cmdtp);

	if (argc > 2) {
		target= 0;
		if (!strncmp(argv[2], "ram", 1))
			target = RD_RAM;
		else if (!strncmp(argv[2], "klog", 1))
			target = RD_KLOG;
		else if (!strncmp(argv[2], "dlog", 1))
			target = RD_DLOG;
		else if (!strncmp(argv[2], "blog", 1))
			target = RD_BLOG;
		else if (!strncmp(argv[2], "fb", 1))
			target = RD_FB;
		else
			return cmd_usage(cmdtp);
	}

	if (!strncmp(argv[1], "start", 5)) {
		return ramdump_start(target);
	} else if (!strncmp(argv[1], "save", 4)) {
		ramdump_logo();
		ret = ramdump_save(target);
		ramdump_clean();
		return ret;
	} else if (!strncmp(argv[1], "upload", 6)) {
		ramdump_logo();
		ret = ramdump_upload(target);
		ramdump_clean();
		return ret;
	} else if (!strncmp(argv[1], "check", 5)) {
		return ramdump_check();
	} else if (!strncmp(argv[1], "show", 4)) {
		if (argc == 4)
			index = simple_strtol(argv[3], NULL, 10);

		return ramdump_show(target, index);
	}

	return cmd_usage(cmdtp);
}

U_BOOT_CMD(ramdump, 4, 1, do_ramdump,
	   "Dump the log at kernel lockup/panic",
	   "show klog/dlog - log print on console\n"
	   "ramdump show blog <index[-1(default), 0]> - log print on console\n"
	   "ramdump start - start save and upload\n"
	   "ramdump save <ram/klog/dlog/blog/fb> - log save as file on UMS\n"
	   "ramdump upload - upload log and ram to the host\n"
	   "ramdump check - check header info\n");
