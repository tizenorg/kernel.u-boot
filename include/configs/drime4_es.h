/*
 * Copyright (C) 2011 Samsung Electronics
 * Chanho Park <chanho61.park@samsung.com>
 *
 * Configuation settings for the SAMSUNG DRIMe4 ES board.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/* High Level Configuration Options */
#define CONFIG_ARMV7		1	/* This is an ARM V7 CPU core  */
#define CONFIG_SAMSUNG		1	/* in a SAMSUNG core */
#define CONFIG_DRIME4		1	/* which is in a DRIMe IV */
#define CONFIG_DRIME4_ES	1

#include <asm/arch/cpu.h>		/* get chip and board defs */

#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO

/* DRAM Base */
#define CONFIG_SYS_SDRAM_BASE		0xC0000000

/* Text Base */
#define CONFIG_SYS_TEXT_BASE		0xC4800000

#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_CMDLINE_TAG
#define CONFIG_INITRD_TAG
#define CONFIG_CMDLINE_EDITING

/*
 * Size of malloc() pool
 * 1MB = 0x100000, 0x100000 = 1024 * 1024
 */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + (1 << 20))
#define CONFIG_SYS_GBL_DATA_SIZE	128	/* size in bytes for */
						/* initial data */

/* PL011 Serial */
#define CONFIG_PL011_SERIAL
#define CONFIG_PL011_CLOCK	100000000
#define CONFIG_PL01x_PORTS	{(void *)CONFIG_SYS_SERIAL0 }
#define CONFIG_CONS_INDEX	0

#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }
#define CONFIG_SYS_SERIAL0		DRIME4_UART_BASE

/* It should define before config_cmd_default.h */
#define CONFIG_SYS_NO_FLASH		1

/* Command definition */
#include <config_cmd_default.h>

#undef CONFIG_CMD_LOADB
#undef CONFIG_CMD_LOADS
#undef CONFIG_CMD_FPGA
#undef CONFIG_CMD_MISC
#undef CONFIG_CMD_NET
#undef CONFIG_CMD_NFS
#undef CONFIG_CMD_XIMG
#define CONFIG_CMD_CACHE
#define CONFIG_CMD_REGINFO
#define CONFIG_CMD_MTDPARTS
#define CONFIG_CMD_MMC
#define CONFIG_CMD_FAT
#define CONFIG_CMD_MBR

#define CONFIG_BOOTDELAY		0
#define CONFIG_ZERO_BOOTDELAY_CHECK

#define CONFIG_FAT_WRITE

#define CONFIG_MTD_DEVICE
#define CONFIG_MTD_PARTITIONS

/* Keep L2 Cache Disabled */
//#define CONFIG_L2_OFF			1

/* Actual modem binary size is 16MiB. Add 2MiB for bad block handling */
#define MTDIDS_DEFAULT		"onenand0=onenand-flash"
#define MTDPARTS_DEFAULT	"mtdparts=onenand-flash:1m(bootloader)"\
				",256k(params)"\
				",2816k(config)"\
				",7m(kernel)"\
				",1m(log)"\
				",-(UBI)\0"

#define NORMAL_MTDPARTS_DEFAULT MTDPARTS_DEFAULT

#define CONFIG_BOOTCOMMAND	"run ramboot"

#define CONFIG_DEFAULT_CONSOLE	"console=ttyAMA0,115200n8\0"

#define CONFIG_RAMDISK_BOOT	"root=/dev/ram0 rw rootfstype=ext2" \
		" ${console} ${meminfo}"

#define CONFIG_COMMON_BOOT	"${console} ${meminfo} ${mtdparts}"

#define CONFIG_BOOTARGS	"root=/dev/mtdblock8 ubi.mtd=5" \
		" rootfstype=cramfs " CONFIG_COMMON_BOOT

#define CONFIG_UPDATEB	"updateb=onenand erase 0x0 0x100000;" \
			" onenand write 0xC2008000 0x0 0x100000\0"

#define CONFIG_UBI_MTD	" ubi.mtd=${ubiblock}"

#define CONFIG_UBIFS_OPTION	"rootflags=bulk_read,no_chk_data_crc"

#define CONFIG_ENV_OVERWRITE
#define CONFIG_SYS_CONSOLE_IS_IN_ENV
#define CONFIG_EXTRA_ENV_SETTINGS					\
	CONFIG_UPDATEB \
	"updatek=" \
		"onenand erase 0x400000 0x600000;" \
		"onenand write 0xC1008000 0x400000 0x600000\0" \
	"updateu=" \
		"onenand erase 0x00C00000 0x07400000;" \
		"onenand write 0xC2000000 0x00C00000 0x8C0000\0" \
	"bootk=" \
		"onenand read 0xC0007FC0 0x400000 0x600000;" \
		"bootm 0xC0007FC0\0" \
	"flashboot=" \
		"set bootargs root=/dev/mtdblock${bootblock} " \
		"rootfstype=${rootfstype}" CONFIG_UBI_MTD " ${opts} " \
		"${lcdinfo} " CONFIG_COMMON_BOOT "; run bootk\0" \
	"ubifsboot=" \
		"set bootargs root=ubi0!rootfs rootfstype=ubifs " \
		CONFIG_UBIFS_OPTION CONFIG_UBI_MTD " ${opts} ${lcdinfo} " \
		CONFIG_COMMON_BOOT "; run bootk\0" \
	"tftpboot=" \
		"set bootargs root=ubi0!rootfs rootfstype=ubifs " \
		CONFIG_UBIFS_OPTION CONFIG_UBI_MTD " ${opts} ${lcdinfo} " \
		CONFIG_COMMON_BOOT "; tftp 0xC0007FC0 uImage; " \
		"bootm 0xC0007FC0\0" \
	"ramboot=" \
		"set bootargs " "console=ttyAMA0,115200n8 mem=512M@0xC0000000;" \
		"bootm 0xC0007FC0\0" \
	"mmcboot=set bootargs root=/dev/mmcblk${mmcdev}p${mmcrootpart} ${lpj} " \
		"rootwait ${console} ${meminfo} ${opts} ${lcdinfo}; " \
		"run loaduimage; bootm 0xC0007FC0\0" \
	"lpj=lpj=2351104\0" \
	"boottrace=setenv opts initcall_debug; run bootcmd\0" \
	"bootchart=set opts init=/sbin/bootchartd; run bootcmd\0" \
	"verify=n\0" \
	"rootfstype=cramfs\0" \
	"console=" CONFIG_DEFAULT_CONSOLE \
	"mtdparts=" MTDPARTS_DEFAULT \
	"meminfo=mem=512M\0" \
	"loaduimage=fatload mmc ${mmcdev}:${mmcbootpart} 0xC0007FC0 uImage\0" \
	"mmcdev=0\0" \
	"mmcbootpart=1\0" \
	"mmcblk=/dev/mmcblk1p1\0" \
	"bootblock=9\0" \
	"ubiblock=5\0" \
	"ubi=enabled\0" \
	"opts=always_resume=1"

/* Miscellaneous configurable options */
#define CONFIG_SYS_LONGHELP		/* undef to save memory */
#define CONFIG_SYS_HUSH_PARSER		/* use "hush" command parser	*/
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#define CONFIG_SYS_PROMPT	"D4 # "
#define CONFIG_SYS_CBSIZE	256	/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE	384	/* Print Buffer Size */
#define CONFIG_SYS_MAXARGS	16	/* max number of command args */
/* Boot Argument Buffer Size */
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE
/* memtest works on */
#define CONFIG_SYS_MEMTEST_START	CONFIG_SYS_SDRAM_BASE
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_SDRAM_BASE + 0x5000000)
#define CONFIG_SYS_LOAD_ADDR		(CONFIG_SYS_SDRAM_BASE + 0xE800000)

#define CONFIG_SYS_HZ			1000

/* Stack sizes */
#define CONFIG_STACKSIZE	(256 << 10)	/* 256 KiB */

/* Goni has 3 banks of DRAM, but swap the bank */
#define CONFIG_NR_DRAM_BANKS	1
#define PHYS_SDRAM_1		CONFIG_SYS_SDRAM_BASE	/* OneDRAM Bank #0 */
#define PHYS_SDRAM_1_SIZE	(512 << 20)		/* 512 MB in Bank #0 */

#define CONFIG_SYS_MONITOR_BASE		0x00000000
#define CONFIG_SYS_MONITOR_LEN		(256 << 10)	/* 256 KiB */

/* FLASH and environment organization */
#define CONFIG_ENV_IS_NOWHERE		1
#define CONFIG_ENV_SIZE			(256 << 10)	/* 256 KiB, 0x40000 */
#define CONFIG_ENV_ADDR			(1 << 20)	/* 1 MB, 0x100000 */

#define CONFIG_DOS_PARTITION		1

/* USB Downloader */
#define CONFIG_SYS_DOWN_ADDR	CONFIG_SYS_SDRAM_BASE
#define CONFIG_RAMDISK_ADDR	(CONFIG_SYS_SDRAM_BASE + 0x03000000)

#define CONFIG_SYS_SP_ADDR	(CONFIG_SYS_LOAD_ADDR - 0x1000000)
#define CONFIG_SYS_INIT_SP_ADDR	(CONFIG_SYS_SP_ADDR - CONFIG_SYS_GBL_DATA_SIZE)

/* MMC */
#if 1
#define CONFIG_GENERIC_MMC	1
#define CONFIG_DW_MMC		1
#define CONFIG_MMC		1
#endif

#endif	/* __CONFIG_H */
