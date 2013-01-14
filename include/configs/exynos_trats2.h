/*
 * Copyright (C) 2011 Samsung Electronics
 * Sanghee Kim <sh0130.kim@samsung.com>
 *
 * Configuation settings for the SAMSUNG TRATS2 board.
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
#define CONFIG_ARMV7		1	/* This is an ARM V7 CPU core */
#define CONFIG_SAMSUNG		1	/* in a SAMSUNG core */
#define CONFIG_S5P		1	/* which is in a S5P Family */
#define CONFIG_EXYNOS4		1	/* which is in a EXYNOS4XXX */
#define CONFIG_SLP_SIG		1	/* make sinature header */
#define CONFIG_SBOOT		1	/* use the s-boot */

#include <version.h>			/* get u-boot version */
#include <timestamp.h>
#include <asm/arch/cpu.h>		/* get chip and board defs */

#define CONFIG_CLK_ARM_800		/* APLL : 800MHz */
#define CONFIG_CLK_BUS_DMC_200_400	/* bus clock: 200Mhz, DMC clock 400Mhz */

#define CONFIG_ARCH_CPU_INIT
#define CONFIG_SKIP_LOWLEVEL_INIT	/* u-boot is loaded by a s-boot */

#define CONFIG_USE_ARCH_MEMSET
#define CONFIG_USE_ARCH_MEMCPY

#define CONFIG_SYS_CACHELINE_SIZE	32

/* Keep L2 Cache Disabled */
#define CONFIG_L2_OFF		1

/*
 * Memory Map
 ********************************************
 * 0x43c00000 ~ 0x456fffff ( 26MB) : MFC0
 ********************************************
 * ..
 * 0x43d00000 ~ 	   ( 64KB) : s-boot
 * ..
 * 0x43e00000 ~ 	   (512KB) : u-boot
 * ..
 * 0x44c00000 ~ 0x450fffff (  5MB) : fb2
 * 0x45100000 ~ 0x4510ffff ( 64KB) : blog
 * 0x45110000 ~ 0x45110fff (  4KB) : pit
 * 0x45111000 ~ 0x456fffff (  5MB) : free
 * ..
 * 0x52000000 ~ 	   (24 MB) : fb
 * ..
 * 0x53000000 ~            (208MB) : download
 */
#define CONFIG_NR_DRAM_BANKS	4
#define PHYS_SDRAM_1		0x40000000	/* LDDDR2 DMC 0 */
#define PHYS_SDRAM_1_SIZE	(256 << 20)	/* 256 MB in CS 0 */
#define PHYS_SDRAM_2		0x50000000	/* LPDDR2 DMC 1 */
#define PHYS_SDRAM_2_SIZE	(256 << 20)	/* 256 MB in CS 0 */
#define PHYS_SDRAM_3		0x60000000	/* LPDDR2 DMC 1 */
#define PHYS_SDRAM_3_SIZE	(256 << 20)	/* 256 MB in CS 0 */
#define PHYS_SDRAM_4		0x70000000	/* LPDDR2 DMC 1 */
#define PHYS_SDRAM_4_SIZE	(256 << 20)	/* 256 MB in CS 0 */
#define PHYS_SDRAM_END		0x80000000

/* TOP_HIDE_BASE: end of MFC0 - free (11MB) */
#define TOP_HIDE_BASE	(0x45700000 - (11 << 20))
#define CONFIG_SYS_MEM_TOP_HIDE	(PHYS_SDRAM_END - TOP_HIDE_BASE)	/* use mfc0 area */

#define CONFIG_SYS_SDRAM_BASE	(PHYS_SDRAM_1)
#define CONFIG_SYS_DOWN_ADDR	0x53000000
#define CONFIG_SYS_BACKUP_ADDR	(CONFIG_SYS_DOWN_ADDR + (80 << 20))
#define CONFIG_SYS_TEXT_BASE	0x43e00000
#define CONFIG_SYS_FB2_ADDR	(PHYS_SDRAM_END - CONFIG_SYS_MEM_TOP_HIDE)
#define CONFIG_SYS_BLOG_ADDR	(CONFIG_SYS_FB2_ADDR + (5 << 20))
#define CONFIG_PIT_DOWN_ADDR	(CONFIG_SYS_BLOG_ADDR + (64 << 10))
#define CONFIG_SYS_RSVD_ADDR	(CONFIG_PIT_DOWN_ADDR + (4 << 10))
#define CONFIG_SYS_FREE_ADDR	(CONFIG_SYS_RSVD_ADDR + (1 << 20))
#define CONFIG_SYS_FB_ADDR	0x52000000

#define CONFIG_SYS_SEC_CONTEXT	0x2023000
#define CONFIG_SYS_NS_BASE	0x204fc00

#define CONFIG_SYS_CLK_FREQ	24000000

#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_CMDLINE_TAG
#define CONFIG_INITRD_TAG
#define CONFIG_SERIAL_TAG
#define CONFIG_REVISION_TAG

/*
 * Architecture magic and machine type
 */
#define MACH_TYPE_SMDK4412	3765

#define CONFIG_DISPLAY_CPUINFO

/*
 * Size of malloc() pool
 */
#define CONFIG_SYS_MALLOC_LEN	(CONFIG_ENV_SIZE + (5 << 20))

/*
 * select serial console configuration
 */
#define CONFIG_SERIAL_MULTI	1
#define CONFIG_SERIAL2		1
#define CONFIG_SILENT_CONSOLE	1	/* enable silent startup */

/* INFORM0~3 registers are cleared by asserting XnRESET pin */
/* INFORM4~7 registers are cleared only by power-up reset */
#define CONFIG_INFO_ADDRESS	0x10020810	/* INFORM4 */
#define CONFIG_INFORM_ADDRESS	0x1002080C	/* INFORM3 */
#define CONFIG_LPM_INFORM	0x10020808      /* INFORM2 */

#define CONFIG_SYS_HUSH_PARSER		/* use "hush" command parser	*/
#ifdef CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#endif

#define CONFIG_CMDLINE_EDITING

#define CONFIG_BAUDRATE		115200

/* It should define before config_cmd_default.h */
#define CONFIG_SYS_NO_FLASH	1

/***********************************************************
 * Command definition
 ***********************************************************/
#include <config_cmd_default.h>

#undef CONFIG_CMD_BOOTD
#undef CONFIG_CMD_CONSOLE
#undef CONFIG_CMD_ECHO
#undef CONFIG_CMD_FPGA
#undef CONFIG_CMD_ITEST
#undef CONFIG_CMD_FLASH
#undef CONFIG_CMD_IMLS
#undef CONFIG_CMD_LOADB
#undef CONFIG_CMD_LOADS
#undef CONFIG_CMD_NAND
#undef CONFIG_CMD_MISC
#undef CONFIG_CMD_NFS
#undef CONFIG_CMD_SOURCE
#undef CONFIG_CMD_XIMG
#define CONFIG_CMD_CACHE
#define CONFIG_CMD_REGINFO
#define CONFIG_CMD_I2C
#define CONFIG_CMD_MMC
#define CONFIG_CMD_SLEEP
#define CONFIG_CMD_DEVICE_POWER
#define CONFIG_CMD_FAT
#define CONFIG_CMD_GPT
#define CONFIG_CMD_PMIC

#undef CONFIG_CRC16

#define CONFIG_BOOTDELAY	0
#define CONFIG_ZERO_BOOTDELAY_CHECK

#define CONFIG_FAT_WRITE

/* To enable CONFIG_CMD_EXT4, CONFIG_CMD_EXT2 should be supported */
#define CONFIG_CMD_EXT2		1
#define CONFIG_CMD_EXT4		1

/* To use the TFTPBOOT over USB, Please enable the CONFIG_CMD_NET */
#undef CONFIG_CMD_NET

/* MMC */
#define CONFIG_GENERIC_MMC	1
#define CONFIG_MMC		1
#define CONFIG_S5P_MMC		1
#define CONFIG_MMC_ASYNC	1
#define CONFIG_MMC_DEFAULT_DEV	0

#include <mobile/parts.h>

#if defined(CONFIG_CMD_EXT4)
#define FSTYPE_DEFAULT		"ext4"
#elif defined(CONFIG_FAT_WRITE)
#define FSTYPE_DEFAULT		"fat"
#else
#error "plz define default filesystem"
#endif

#if defined(CONFIG_OFFICIAL_REL)
#define UARTPATH_DEFAULT	"cp"
#define SILENT_DEFAULT		"on"
#define RAMDUMP_DEFAULT		""
#else
#define UARTPATH_DEFAULT	"ap"
#define SILENT_DEFAULT		""
#define RAMDUMP_DEFAULT		"both"	/* save, upload, both */
#endif

#define MBRPARTS_DEFAULT	"8M("PARTS_CSA")"\
				",60M("PARTS_BOOT")"\
				",60M("PARTS_QBOOT")"\
				",1G("PARTS_ROOT")"\
				",3G("PARTS_DATA")"\
				",150M("PARTS_CSC")"\
				",-("PARTS_UMS")\0"


#define CONFIG_BOOTARGS		"Please use defined boot"
#define CONFIG_BOOTCOMMAND	"run mmcboot"
#define CONFIG_DEFAULT_CONSOLE	"console=ttySAC2,115200n8\0"

#define CSAPART_DEFAULT		"/dev/mmcblk0p1"
#define CONFIG_SLP_PART_ENV	"SLP_VAR_PART=csa " CSAPART_DEFAULT " /csa ext4 data=journal\0"

#define CONFIG_SLP_EXTRA_ENV 	"SLP_LCD_LEVEL=0\0" \
				"SLP_SWITCH_SEL=0\0" \
				"SLP_NATION_SEL=0\0" \
				"SLP_ROOTFS_NEW=0\0" \
				"SLP_KERNEL_NEW=0\0" \
				"SLP_FLAG_RTL=0\0" \
				"SLP_FLAG_FUS=0\0" \
				"SLP_FLAG_FOTA=0\0" \
				"SLP_FLAG_EFS_CLEAR=0\0"

#define CONFIG_ENV_OVERWRITE
#define CONFIG_ENV_AUTOSAVE
#define CONFIG_SYS_CONSOLE_INFO_QUIET
#define CONFIG_SYS_CONSOLE_IS_IN_ENV

#define CONFIG_EXTRA_ENV_SETTINGS \
	"bootk=run loaduimage; bootm 0x40007FC0\0" \
	"updatemmc=mmc boot 0 1 1 1; mmc write 0 0x50800000 0 0x400;" \
		" mmc boot 0 1 1 0\0" \
	"updatebackup=mmc boot 0 1 1 2; mmc write 0 0x51000000 0 0x400;" \
		" mmc boot 0 1 1 0\0" \
	"updatebootb=mmc read 0 0x51000000 0x80 0x400; run updatebackup\0" \
	"updateuboot=mmc write 0 0x50000000 0x80 0x400\0" \
	"updaterestore=mmc boot 0 1 1 2; mmc read 0 0x50000000 0 0x400;" \
		"mmc boot 0 1 1 0; run updateuboot\0" \
	"setupboot=run updatemmc; run updateuboot; run updatebootb\0" \
	"mmcboot=set bootargs root=/dev/mmcblk0p5 rw rootfstype=ext4 rootwait " \
		"${console} ${meminfo} ${debuginfo} " \
		"${bootmode} ${lcdinfo} ${muicpathinfo} ${lpjinfo} ${opts} ${csamount}; " \
		"run loaduimage; bootm 0x40007FC0\0" \
	"bootchart=set opts init=/sbin/bootchartd; run bootcmd\0" \
	"boottrace=setenv opts initcall_debug; run bootcmd\0" \
	"verify=n\0" \
	"rootfstype=ext4\0" \
	"console=" CONFIG_DEFAULT_CONSOLE \
	"mbrparts=" MBRPARTS_DEFAULT \
	"meminfo=fbmem=" MK_STR(CONFIG_FB_SIZE) "M@" MK_STR(CONFIG_FB_ADDR) "\0" \
	"kernelname=uImage\0" \
	"loaduimage=" FSTYPE_DEFAULT "load mmc ${mmcdev}:${mmcbootpart} 0x40007FC0 ${kernelname}\0" \
	"mmcdev=0\0" \
	"mmcbootpart=2\0" \
	"mmcrootpart=3\0" \
	"csamount=csa=" CSAPART_DEFAULT "\0" \
	"opts=resume=179:3\0" \
	"uartpath=" UARTPATH_DEFAULT "\0" \
	"usbpath=ap\0" \
	"silent=" SILENT_DEFAULT "\0" \
	"ramdump=" RAMDUMP_DEFAULT "\0" \
	"ver=" U_BOOT_VERSION" (" U_BOOT_DATE " - " U_BOOT_TIME ")\0" \
	CONFIG_SLP_PART_ENV \
	CONFIG_SLP_EXTRA_ENV

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_LONGHELP	/* undef to save memory */
#define CONFIG_SYS_PROMPT	"TRATS2 # "	/* Monitor Command Prompt */
#define CONFIG_SYS_CBSIZE	256	/* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE	384	/* Print Buffer Size */
#define CONFIG_SYS_MAXARGS	32	/* max number of command args */
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE	/* Boot Argument Buffer Size */

#define CONFIG_SYS_MEMTEST_START	CONFIG_SYS_SDRAM_BASE	/* memtest works on	      */
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_SDRAM_BASE + 0x5000000)
#define CONFIG_SYS_LOAD_ADDR		CONFIG_SYS_TEXT_BASE

#define CONFIG_SYS_HZ		1000

/* valid baudrates */
#define CONFIG_SYS_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }

/*-----------------------------------------------------------------------
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE	(256 << 10)	/* regular stack 256KB */

#define CONFIG_SYS_MONITOR_BASE	0x00000000

/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */

#define CONFIG_SYS_MONITOR_LEN	(256 << 10)	/* Reserve 2 sectors */

#define CONFIG_ENV_IS_IN_MMC	1
#define CONFIG_SYS_MMC_ENV_DEV	CONFIG_MMC_DEFAULT_DEV
#define CONFIG_ENV_SIZE		0x4000
#define CONFIG_ENV_OFFSET	0x310000
#define CONFIG_ENV_UPDATE_WITH_DL	1
#define CONFIG_EFI_PARTITION	1

#define CONFIG_CMD_PIT
#define CONFIG_PIT_IS_IN_MMC	1
#define CONFIG_SYS_MMC_PIT_DEV	CONFIG_MMC_DEFAULT_DEV
#define CONFIG_PIT_DEFAULT_ADDR	0x8000	/* block */
#define CONFIG_PIT_DEFAULT_SIZE	0x1000

#define CONFIG_MISC_INIT_R
#define CONFIG_BOARD_EARLY_INIT_F

/* I2C */
#include <i2c-gpio.h>
#define CONFIG_S5P_GPIO_I2C	1
#define CONFIG_SYS_I2C_SPEED   400000          /* 400 Kbps */
#define CONFIG_SYS_I2C_SLAVE    0x10
#define CONFIG_SYS_I2C_INIT_BOARD
#define CONFIG_I2C_MULTI_BUS
#define CONFIG_MULTI_I2C
#define CONFIG_HARD_I2C
#define CONFIG_DRIVER_S3C24X0_I2C
#define CONFIG_MAX_I2C_NUM     8
#define CONFIG_SOFT_I2C
#define CONFIG_SOFT_I2C_READ_REPEATED_START
#define CONFIG_SYS_MAX_I2C_BUS  11

/* POWER */
#define CONFIG_PMIC_MAX77686
#define CONFIG_PMIC_MAX77693

/* USB Downloader */
#define CONFIG_CMD_USBDOWN
#define	CONFIG_S5P_USB_DMA	1	/* DMA mode */
#define CONFIG_USB_DEVGURU	1	/* USB driver */
#define CONFIG_S5P_USB_NON_ZLP	1	/* NON-ZLP mode on DEVGURU */
#define CONFIG_USBD_PROG_IMAGE	1	/* progress bar option */

/* PWM */
#define CONFIG_PWM		1

/* LCD */
#define CONFIG_LCD		1
#define CONFIG_FB_ADDR		CONFIG_SYS_FB_ADDR
#define CONFIG_FB_SIZE		24
#define CONFIG_S5PC1XXFB	1
#define CONFIG_S6E8AX0		1
#define CONFIG_DSIM		1
/* Insert bmp animation compressed */
#define CONFIG_CMD_BMP
#define CONFIG_BMP_16BPP
#define CONFIG_BMP_32BPP
#define CONFIG_VIDEO_BMP_GZIP
#define CONFIG_SYS_VIDEO_LOGO_MAX_SIZE	(700*820*4)

/* RAM DUMP */
#define CONFIG_CMD_RAMDUMP	1
#define CONFIG_SYS_GETLOG_ADDR	(PHYS_SDRAM_1 + 0x3000) /* 4KB, GETLOG */
#define CONFIG_SYS_MAGIC_ADDR	CONFIG_SYS_GETLOG_ADDR	/* use log's magic */

/* Bootloader LOG */
#define CONFIG_LOGGER		1

#if 0
/* Secure Boot */
#define CONFIG_SECURE_BOOTING	1
#define CONFIG_CHECK_CUSTOM	1

/* TrustZone */
#define CONFIG_TRUSTZONE	1
#define CONFIG_TRUSTZONE_RESERVED_DRAM	(1 << 20)

/* FOTA */
#define CONFIG_FOTA
#define CONFIG_FOTA_DELTA_DOWNLOAD
#endif

/* ETC */
#define CONFIG_STOPWATCH	1
#define CONFIG_RESET_FLAG	1	/* for watchdog reset flag */
#define CONFIG_SLP_NEW_HEADER	1	/* for SLP mkheader */
#define CONFIG_CMD_FSTYPE	1

#define CONFIG_SYS_INIT_SP_ADDR	(CONFIG_SYS_LOAD_ADDR \
				 - GENERATED_GBL_DATA_SIZE)

#endif	/* __CONFIG_H */
