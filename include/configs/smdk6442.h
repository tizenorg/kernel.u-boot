/*
 * Copyright (C) 2009 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 *
 * Configuation settings for the SAMSUNG board.
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

/*
 * High Level Configuration Options
 * (easy to change)
 */
#define CONFIG_SAMSUNG		1	/* in a SAMSUNG core */
#define CONFIG_S5P		1	/* which is in a S5P Family */
#define CONFIG_S5P6442		1	/* which is in a S5P6442 */
#define CONFIG_S5P6442_EVT1	1	/* which is in a S5P6442 EVT1 */

#include <asm/arch/cpu.h>		/* get chip and board defs */

#define CONFIG_ARCH_CPU_INIT

#define CONFIG_SYS_SDRAM_BASE	0x30000000

/* input clock of PLL: SMDK6442 has 12MHz input clock */
#define CONFIG_SYS_CLK_FREQ_6442	12000000

#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_CMDLINE_TAG
#define CONFIG_INITRD_TAG
//#define CONFIG_REVISION_TAG

/*
 * Architecture magic and machine type
 */
#define CONFIG_DISPLAY_CPUINFO

#undef CONFIG_SKIP_RELOCATE_UBOOT

/*
 * Size of malloc() pool
 */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + 1024 * 1024)
#define CONFIG_SYS_GBL_DATA_SIZE	128	/* size in bytes for initial data */

/*
 * select serial console configuration
 */
#define CONFIG_SERIAL_MULTI	1
#define CONFIG_SERIAL1          1	/* we use SERIAL 1 on S5P6442 */

#if 0
/* MMC */
#define CONFIG_GENERIC_MMC	1
#define CONFIG_MMC		1
#define CONFIG_S5P_MMC		1
#define CONFIG_MMC_INDEX	0
#endif

#define CONFIG_SYS_HUSH_PARSER		/* use "hush" command parser	*/
#ifdef CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#endif

#define CONFIG_CMDLINE_EDITING

#define CONFIG_BAUDRATE		115200

/* It should define before config_cmd_default.h */
#define CONFIG_SYS_NO_FLASH		1
/***********************************************************
 * Command definition
 ***********************************************************/
#include <config_cmd_default.h>

#undef CONFIG_CMD_LOADB
#undef CONFIG_CMD_LOADS
#undef CONFIG_CMD_BOOTD
#undef CONFIG_CMD_FPGA
#undef CONFIG_CMD_XIMG
#undef CONFIG_CMD_NAND
#undef CONFIG_CMD_IMLS
#undef CONFIG_CMD_FLASH
#undef CONFIG_CMD_IMLS
#undef CONFIG_CMD_NET
#define CONFIG_CMD_CACHE
#define CONFIG_CMD_REGINFO
#define CONFIG_CMD_ONENAND
#define CONFIG_CMD_MTDPARTS
#if 0
#define CONFIG_CMD_I2C
#define CONFIG_CMD_MMC
#define CONFIG_CMD_SLEEP
#define CONFIG_CMD_GPIO
#define CONFIG_CMD_PMIC
#define CONFIG_CMD_DEVICE_POWER
#endif

#define CONFIG_SYS_64BIT_VSPRINTF	1

#define CONFIG_BOOTDELAY	1

#define CONFIG_ZERO_BOOTDELAY_CHECK

#define CONFIG_NETMASK		255.255.255.0
#define CONFIG_IPADDR		192.168.129.3
#define CONFIG_SERVERIP		192.168.129.1
#define CONFIG_GATEWAYIP	192.168.129.1
#define CONFIG_ETHADDR		00:0E:99:34:10:00

#define CONFIG_MTD_DEVICE
#define CONFIG_MTD_PARTITIONS

/* Actual modem binary size is 16MiB. Add 2MiB for bad block handling */
#define MTDIDS_DEFAULT		"onenand0=samsung-onenand"
#define MTDPARTS_DEFAULT	"mtdparts=samsung-onenand:256k(bootloader)"\
				",128k(params)"\
				",3m(kernel)"\
				",18m(modem)"\
				",7m(fota)"\
				",8m(csa)"\
				",1m(log)"\
				",-(UBI)\0"

#define MTDPARTS_DEFAULT_4KB	"mtdparts=samsung-onenand:256k(bootloader)"\
				",256k(params)"\
				",3m(kernel)"\
				",18m(modem)"\
				",7m(fota)"\
				",8m(csa)"\
				",1m(log)"\
				",-(UBI)\0"

#define NORMAL_MTDPARTS_DEFAULT MTDPARTS_DEFAULT

#define CONFIG_BOOTCOMMAND	"run ubifsboot"

#define CONFIG_RAMDISK_BOOT	"root=/dev/ram0 rw rootfstype=ext2" \
		" console=ttySAC1,115200n8" \
		" ${meminfo}"

#define CONFIG_COMMON_BOOT	"console=ttySAC1,115200n8" \
		" ${meminfo}" \
		" ${mtdparts}"

#define CONFIG_BOOTARGS	"root=/dev/mtdblock8 ubi.mtd=7 ubi.mtd=5" \
		" rootfstype=cramfs " CONFIG_COMMON_BOOT

#define CONFIG_UPDATEB	"updateb=onenand erase 0x0 0x40000;" \
			" onenand write 0x32008000 0x0 0x40000\0"

#define CONFIG_UBI_MTD	" ubi.mtd=${ubiblock} ubi.mtd=5"

#define CONFIG_ENV_OVERWRITE
#define CONFIG_SYS_CONSOLE_IS_IN_ENV
#define CONFIG_EXTRA_ENV_SETTINGS					\
	CONFIG_UPDATEB \
	"updatek=onenand erase 0x80000 0x300000;" \
	" onenand write 0x31008000 0x80000 0x300000\0" \
	"updateu=onenand erase 0x01560000 0x1eaa0000;" \
	" onenand write 0x32000000 0x1260000 0x8C0000\0" \
	"bootk=onenand read 0x30007FC0 0x80000 0x300000;" \
	" bootm 0x30007FC0\0" \
	"flashboot=set bootargs root=/dev/mtdblock${bootblock}" \
	 " rootfstype=${rootfstype}" \
	CONFIG_UBI_MTD " ${opts} ${lcdinfo} " CONFIG_COMMON_BOOT "; run bootk\0" \
	"ubifsboot=set bootargs root=ubi0!rootfs rootfstype=ubifs" \
	CONFIG_UBI_MTD " ${opts} ${lcdinfo} " CONFIG_COMMON_BOOT "; run bootk\0" \
	"boottrace=setenv opts initcall_debug; run bootcmd\0" \
	"android=set bootargs root=ubi0!ramdisk " CONFIG_UBI_MTD \
	 " rootfstype=ubifs init=/init.sh " CONFIG_COMMON_BOOT "; run bootk\0" \
	"nfsboot=set bootargs root=/dev/nfs rw " CONFIG_UBI_MTD \
	 " nfsroot=${nfsroot},nolock,tcp ip=${ipaddr}:${serverip}:${gatewayip}:" \
	 "${netmask}:generic:usb0:off " CONFIG_COMMON_BOOT "; run bootk\0" \
	"ramboot=set bootargs " CONFIG_RAMDISK_BOOT \
	 " initrd=0x33000000,8M ramdisk=8192\0" \
	"mmcboot=set bootargs root=${mmcblk} rootfstype=${rootfstype}" \
	 CONFIG_UBI_MTD " ${opts} ${lcdinfo} " CONFIG_COMMON_BOOT "; run bootk\0" \
	"verify=n\0" \
	"rootfstype=cramfs\0" \
	"mtdparts=" MTDPARTS_DEFAULT \
	"meminfo=mem=80M mem=256M@0x40000000\0" \
	"nfsroot=/nfsroot/arm\0" \
	"mmcblk=/dev/mmcblk1p1\0" \
	"bootblock=8\0" \
	"ubiblock=7\0" \
	"ubi=enabled"

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP	/* undef to save memory */
#define CONFIG_SYS_PROMPT	"SMDK6442 # "	/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE	256	/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE	384	/* Print Buffer Size */
#define CONFIG_SYS_MAXARGS	16	/* max number of command args */
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE	/* Boot Argument Buffer Size */

#define CONFIG_SYS_MEMTEST_START	CONFIG_SYS_SDRAM_BASE	/* memtest works on	      */
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_SDRAM_BASE + 0x5000000)

#define CONFIG_SYS_LOAD_ADDR		(CONFIG_SYS_SDRAM_BASE + 0x4800000)

#define CONFIG_SYS_HZ			1000

/* valid baudrates */
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }

/* base address for uboot */
#define CONFIG_SYS_PHY_UBOOT_BASE	(CONFIG_SYS_SDRAM_BASE + 0x04800000)
#define CONFIG_SYS_UBOOT_BASE		(CONFIG_SYS_SDRAM_BASE + 0x04800000)

/*-----------------------------------------------------------------------
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE	(256 << 10)	/* regular stack 256KB */

/* S5P6442 has 2 banks of DRAM */
#define CONFIG_NR_DRAM_BANKS	2

/* SDRAM Bank #1 */
#define PHYS_SDRAM_1		CONFIG_SYS_SDRAM_BASE
#define PHYS_SDRAM_1_SIZE	(80 << 20)		/* 80 MB in Bank #1 */

/* SDRAM Bank #2 */
#define PHYS_SDRAM_2		CONFIG_SYS_SDRAM_BASE + 0x10000000
#define PHYS_SDRAM_2_SIZE	(256 << 20)		/* 256 MB in Bank #2 */

#define CONFIG_SYS_MONITOR_BASE	0x00000000

/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */

#define CONFIG_SYS_MONITOR_LEN		(256 << 10)	/* Reserve 2 sectors */

/* OneNAND IPL uses 8KiB */
#ifdef CONFIG_S5P6442_EVT1
#define CONFIG_ONENAND_START_PAGE	2
#else
#define CONFIG_ONENAND_START_PAGE	4
#endif

#define CONFIG_ENV_IS_IN_ONENAND	1
#define CONFIG_ENV_SIZE			(256 << 10)	/* 256 KiB, 0x40000 */
#define CONFIG_ENV_ADDR			(256 << 10)	/* 256 KiB, 0x40000 */
#define CONFIG_ENV_OFFSET		(256 << 10)	/* 256 KiB, 0x40000 */

#define CONFIG_USE_ONENAND_BOARD_INIT
#define CONFIG_SAMSUNG_ONENAND		1
#define CONFIG_SYS_ONENAND_BASE		0xB0000000

#define CONFIG_DOS_PARTITION	1

//#define CONFIG_MISC_INIT_R

#if 0
/* I2C */
#if 0
#define CONFIG_DRIVER_S5PC1XX_I2C
#define CONFIG_HARD_I2C		1
#define CONFIG_SYS_I2C_SPEED	50000
#define CONFIG_SYS_I2C_SLAVE	0xFE
#define CONFIG_SYS_I2C_0	1
#else
#include <i2c-gpio.h>
#define CONFIG_SOFT_I2C		1
#define CONFIG_SYS_I2C_INIT_BOARD
#define CONFIG_SYS_I2C_SPEED	50000
#define CONFIG_I2C_MULTI_BUS
#define CONFIG_SYS_MAX_I2C_BUS	6
#endif
#endif

/* USB Downloader */
#define CONFIG_CMD_USBDOWN
#define CONFIG_SAMSUNG_USB
#define CONFIG_OTG_CLK_OSCC
#define CONFIG_SYS_DOWN_ADDR	CONFIG_SYS_SDRAM_BASE
#define CONFIG_RAMDISK_ADDR	(CONFIG_SYS_SDRAM_BASE + 0x03000000)

/* LCD */
#if 0		/* For LCD test */
#define CONFIG_LCD		1
#define CONFIG_S5PC1XXFB	1
#endif

#if 0
#define CONFIG_CMD_EXT2			1
#define CONFIG_CMD_ONENAND_EXT2		1
#endif

#endif	/* __CONFIG_H */
