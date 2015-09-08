/*
 *  Copyright (C) 2011 Samsung Electronics
 *  Minkyu Kang <mk7.kang@samsung.com>
 *  Kyungmin Park <kyungmin.park@samsung.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
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
 */
#define CONFIG_ARMV7		1	/* This is an ARM V7 CPU core */
#define CONFIG_OMAP		1	/* in a TI OMAP core */
#define CONFIG_OMAP44XX		1	/* which is a 44XX */
#define CONFIG_OMAP4430		1	/* which is in a 4430 */
#define CONFIG_OMAP4_UNIVERSAL	1	/* working with Universal */
#define CONFIG_ARCH_CPU_INIT

/* Get CPU defs */
#include <asm/arch/cpu.h>
#include <asm/arch/omap4.h>

#define MACH_TYPE	2993		/* universal_c210 + 4 */

/* Display CPU and Board Info */
#define CONFIG_DISPLAY_CPUINFO		1

/* Keep L2 Cache Disabled */
#define CONFIG_L2_OFF			1

/* Clock Defines */
#define V_OSCK			38400000	/* Clock output from T2 */
#define V_SCLK                   V_OSCK

#undef CONFIG_USE_IRQ				/* no support for IRQs */
#define CONFIG_MISC_INIT_R
#define CONFIG_CMDLINE_TAG		1	/* enable passing of ATAGs */
#define CONFIG_SETUP_MEMORY_TAGS	1
#define CONFIG_INITRD_TAG		1
#define CONFIG_REVISION_TAG		1

#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + (256 << 10))
						/* initial data */
/* Vector Base */
#define CONFIG_SYS_CA9_VECTOR_BASE	SRAM_ROM_VECT_BASE

/*
 * Hardware drivers
 */

/*
 * serial port - NS16550 compatible
 */
#define V_NS16550_CLK			48000000

#define CONFIG_SYS_NS16550
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_REG_SIZE	(-4)
#define CONFIG_SYS_NS16550_CLK		V_NS16550_CLK
#define CONFIG_CONS_INDEX		4
#define CONFIG_SYS_NS16550_COM4		UART4_BASE

#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_BAUDRATE_TABLE	{4800, 9600, 19200, 38400, 57600,\
					115200}
/* I2C  */
#define CONFIG_HARD_I2C			1
#define CONFIG_SYS_I2C_SPEED		100000
#define CONFIG_SYS_I2C_SLAVE		1
#define CONFIG_SYS_I2C_BUS		0
#define CONFIG_SYS_I2C_BUS_SELECT	1
#define CONFIG_DRIVER_OMAP34XX_I2C	1
#define CONFIG_I2C_MULTI_BUS		1

/* TWL6030 */
#define CONFIG_TWL6030_POWER		1
#define CONFIG_CMD_BAT			1

/* MMC */
#define CONFIG_GENERIC_MMC		1
#define CONFIG_MMC			1
#define CONFIG_OMAP_HSMMC		1
#define CONFIG_SYS_MMC_SET_DEV		1
#define CONFIG_DOS_PARTITION		1

/* MMC ENV related defines */
#define CONFIG_ENV_IS_IN_MMC		1
#define CONFIG_SYS_MMC_ENV_DEV		1	/* SLOT2: eMMC(1) */
#define CONFIG_ENV_SIZE			4096
#define CONFIG_ENV_OFFSET		((32 - 4) << 10)/* 32KiB - 4KiB */
#define CONFIG_SYS_CONSOLE_IS_IN_ENV	1

/* Flash */
#define CONFIG_SYS_NO_FLASH	1

/* commands to include */
#include <config_cmd_default.h>

/* Enabled commands */
#define CONFIG_CMD_FAT		/* FAT support                  */
#define CONFIG_CMD_I2C		/* I2C serial bus support	*/
#define CONFIG_CMD_MMC		/* MMC support                  */
#define CONFIG_CMD_SAVEENV
#define CONFIG_CMD_MBR
#define CONFIG_CMDLINE_EDITING
#define CONFIG_FAT_WRITE

/* Disabled commands */
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
#undef CONFIG_CRC16
#undef CONFIG_XYZMODEM
#undef CONFIG_CMD_NET

/*
 * Environment setup
 */
#define CONFIG_BOOTDELAY	1

#define MBRPARTS_DEFAULT	"20M(permanent)"\
				",20M(boot)"\
				",1G(system)"\
				",100M(swap)"\
				",-(UMS)\0"

#define CONFIG_BOOTLOADER_SECTOR 0x80

#define CONFIG_BOOTARGS		"Please use defined boot"
#define CONFIG_BOOTCOMMAND	"run mmcboot"
#define CONFIG_DEFAULT_CONSOLE	"console=ttyO3,115200n8\0"
#define CONFIG_ENV_COMMON_BOOT	"${console} ${meminfo}"

#define CONFIG_ENV_OVERWRITE
#define CONFIG_SYS_CONSOLE_INFO_QUIET

#define CONFIG_EXTRA_ENV_SETTINGS					\
	"bootk=run loaduimage; bootm 0x80007FC0\0" \
	"updatemmc=mmc boot 1 1 1 1; mmc write 1 0x82008000 0 0x200;" \
		" mmc boot 1 1 1 0\0" \
	"updatebackup=mmc boot 1 1 1 2; mmc write 1 0x82100000 0 0x200;" \
		" mmc boot 1 1 1 0\0" \
	"updatebootb=mmc read 1 0x82100000 0x80 0x200; run updatebackup\0" \
	"updateuboot=mmc write 1 0x84800000 0x80 0x200\0" \
	"updaterestore=mmc boot 1 1 1 2; mmc read 1 0x84800000 0 0x200;" \
		"mmc boot 1 1 1 0; run updateuboot\0" \
	"setupboot=run updatemmc; run updateuboot; run updatebootb\0" \
	"ramfsboot=set bootargs root=/dev/ram0 rw rootfstype=ext2 " \
		"${console} ${meminfo} " \
		"initrd=0x83000000,8M ramdisk=8192\0" \
	"mmcboot=set bootargs root=/dev/mmcblk0p${mmcrootpart} " \
		"rootwait ${console} ${meminfo} ${opts} ${lcdinfo}; " \
		"run loaduimage; bootm 0x80007FC0\0" \
	"bootchart=set opts init=/sbin/bootchartd; run bootcmd\0" \
	"boottrace=setenv opts initcall_debug; run bootcmd\0" \
	"verify=n\0" \
	"rootfstype=ext4\0" \
	"console=" CONFIG_DEFAULT_CONSOLE \
	"mbrparts=" MBRPARTS_DEFAULT \
	"nfsroot=/nfsroot/arm\0" \
	"loaduimage=fatload mmc ${mmcdev}:${mmcbootpart} 0x80007FC0 uImage\0" \
	"mmcdev=1\0" \
	"mmcbootpart=2\0" \
	"mmcrootpart=3\0" \
	"opts=always_resume=1"

#define CONFIG_AUTO_COMPLETE		1

/*
 * Miscellaneous configurable options
 */

#define CONFIG_SYS_LONGHELP	/* undef to save memory */
#define CONFIG_SYS_HUSH_PARSER	/* use "hush" command parser */
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
#define CONFIG_SYS_PROMPT		"Universal # "
#define CONFIG_SYS_CBSIZE		256
/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + \
					sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_MAXARGS		16
/* Boot Argument Buffer Size */
#define CONFIG_SYS_BARGSIZE		(CONFIG_SYS_CBSIZE)

/*
 * memtest setup
 */
#define CONFIG_SYS_MEMTEST_START	0x80000000
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_MEMTEST_START + (32 << 20))

/* Default load address */
#define CONFIG_SYS_LOAD_ADDR		0x84800000

/* Use General purpose timer 1 */
#define CONFIG_SYS_TIMERBASE		GPT2_BASE
#define CONFIG_SYS_PTV			2	/* Divisor: 2^(PTV+1) => 8 */
#define CONFIG_SYS_HZ			1000

/*
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE	(128 << 10)	/* Regular stack */
#ifdef CONFIG_USE_IRQ
#define CONFIG_STACKSIZE_IRQ	(4 << 10)	/* IRQ stack */
#define CONFIG_STACKSIZE_FIQ	(4 << 10)	/* FIQ stack */
#endif

/*
 * SDRAM Memory Map
 * Even though we use two CS all the memory
 * is mapped to one contiguous block
 */
#define CONFIG_NR_DRAM_BANKS	1

#define CONFIG_SYS_SDRAM_BASE		0x80000000
#define CONFIG_SYS_TEXT_BASE		0x84800000
#define CONFIG_SYS_INIT_RAM_ADDR	0x4030D800
#define CONFIG_SYS_INIT_RAM_SIZE	0x800
#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_INIT_RAM_ADDR + \
					 CONFIG_SYS_INIT_RAM_SIZE - \
					 GENERATED_GBL_DATA_SIZE)

/* USB Downloader */
#define CONFIG_CMD_USBDOWN
#define CONFIG_SAMSUNG_USB
#define CONFIG_SYS_DOWN_ADDR	CONFIG_SYS_SDRAM_BASE
#define CONFIG_INTERFACE_UMTI
#define CONFIG_SBOOT_UPDATE

#endif /* __CONFIG_H */
