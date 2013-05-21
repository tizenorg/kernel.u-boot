/*
 *  Copyright (C) 2012 Samsung Electronics
 *  Rajeshwari Shinde <rajeshwari.s@samsung.com>
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
#include <fdtdec.h>
#include <i2c.h>
#include <power/pmic.h>
#include <power/max77686_pmic.h>
#include <errno.h>

DECLARE_GLOBAL_DATA_PTR;

static const char max77686_buck_addr[] = {
	0xff, 0x10, 0x12, 0x1c, 0x26, 0x30, 0x32, 0x34, 0x36, 0x38
};

static unsigned int max77686_ldo_volt2hex(int ldo, ulong uV)
{
	unsigned int hex = 0;
	const unsigned int max_hex = 0x3f;

	switch (ldo) {
	case 1:
	case 2:
	case 6:
	case 7:
	case 8:
	case 15:
		hex = (uV - 800000) / 25000;
		break;
	default:
		hex = (uV - 800000) / 50000;
	}

	if (0 <= hex && hex <= max_hex)
		return hex;

	debug("%s: %ld is wrong voltage value for LDO%d\n", __func__, uV, ldo);
	return 0;
}

int max77686_set_ldo_voltage(struct pmic *p, int ldo, ulong uV)
{
	unsigned int val, ret, hex, adr, mask;

	if (ldo < 1 && 26 < ldo) {
		printf("%s: %d is wrong ldo number\n", __func__, ldo);
		return -1;
	}

	adr = MAX77686_REG_PMIC_LDO1CTRL1 + ldo - 1;
	mask = 0x3f;
	hex = max77686_ldo_volt2hex(ldo, uV);

	if (!hex)
		return -1;

	ret = pmic_reg_read(p, adr, &val);
	val &= ~mask;
	val |= hex;
	ret |= pmic_reg_write(p, adr, val);

	return ret;
}

int max77686_set_ldo_mode(struct pmic *p, int ldo, char opmode)
{
	unsigned int val, ret, mask, adr, mode;

	if (ldo < 1 && 26 < ldo) {
		printf("%s: %d is wrong ldo number\n", __func__, ldo);
		return -1;
	}

	adr = MAX77686_REG_PMIC_LDO1CTRL1 + ldo - 1;

	/* mask */
	mask = 0xc0;

	/* mode */
	if (opmode == OPMODE_OFF) {
		mode = 0x00;
	} else if (opmode == OPMODE_STANDBY) {
		switch (ldo) {
		case 2:
		case 6:
		case 7:
		case 8:
		case 10:
		case 11:
		case 12:
		case 14:
		case 15:
		case 16:
			mode = 0x40;
			break;
		default:
			mode = 0xff;
		}
	} else if (opmode == OPMODE_LPM) {
		mode = 0x80;
	} else if (opmode == OPMODE_ON) {
		mode = 0xc0;
	} else {
		mode = 0xff;
	}

	if (mode == 0xff) {
		printf("%s: %d is not supported on LDO%d\n",
		       __func__, opmode, ldo);
		return -1;
	}

	ret = pmic_reg_read(p, adr, &val);
	val &= ~mask;
	val |= mode;
	ret |= pmic_reg_write(p, adr, val);

	return ret;
}

int max77686_set_buck_mode(struct pmic *p, int buck, char opmode)
{
	unsigned int val, ret, mask, adr, size, mode;

	size = sizeof(max77686_buck_addr) / sizeof(*max77686_buck_addr);
	if (buck >= size) {
		printf("%s: %d is wrong buck number\n", __func__, buck);
		return -1;
	}

	adr = max77686_buck_addr[buck];

	/* mask */
	switch (buck) {
	case 2:
	case 3:
	case 4:
		mask = 0x30;
		break;
	default:
		mask = 0x03;
	}

	/* mode */
	if (opmode == OPMODE_OFF) {
		mode = 0x00;
	} else if (opmode == OPMODE_STANDBY) {
		switch (buck) {
		case 1:
			mode = 0x01;
			break;
		case 2:
		case 3:
		case 4:
			mode = 0x10;
			break;
		default:
			mode = 0xff;
		}
	} else if (opmode == OPMODE_LPM) {
		switch (buck) {
		case 2:
		case 3:
		case 4:
			mode = 0x20;
			break;
		default:
			mode = 0xff;
		}
	} else if (opmode == OPMODE_ON) {
		switch (buck) {
		case 2:
		case 3:
		case 4:
			mode = 0x30;
			break;
		default:
			mode = 0x03;
		}
	} else {
		mode = 0xff;
	}

	if (mode == 0xff) {
		printf("%s: %d is not supported on BUCK%d\n",
		       __func__, opmode, buck);
		return -1;
	}

	ret = pmic_reg_read(p, adr, &val);
	val &= ~mask;
	val |= mode;
	ret |= pmic_reg_write(p, adr, val);

	return ret;
}

int pmic_init(unsigned char bus)
{
	static const char name[] = "MAX77686_PMIC";
	struct pmic *p = pmic_alloc();

	if (!p) {
		printf("%s: POWER allocation error!\n", __func__);
		return -ENOMEM;
	}

#ifdef CONFIG_OF_CONTROL
	const void *blob = gd->fdt_blob;
	int node, parent;

	node = fdtdec_next_compatible(blob, 0, COMPAT_MAXIM_MAX77686_PMIC);
	if (node < 0) {
		debug("PMIC: No node for PMIC Chip in device tree\n");
		debug("node = %d\n", node);
		return -1;
	}

	parent = fdt_parent_offset(blob, node);
	if (parent < 0) {
		debug("%s: Cannot find node parent\n", __func__);
		return -1;
	}

	p->bus = i2c_get_bus_num_fdt(parent);
	if (p->bus < 0) {
		debug("%s: Cannot find I2C bus\n", __func__);
		return -1;
	}
	p->hw.i2c.addr = fdtdec_get_int(blob, node, "reg", 9);
#else
	p->bus = bus;
	p->hw.i2c.addr = MAX77686_I2C_ADDR;
#endif

	p->name = name;
	p->interface = PMIC_I2C;
	p->number_of_regs = PMIC_NUM_OF_REGS;
	p->hw.i2c.tx_num = 1;

	puts("Board PMIC init\n");

	return 0;
}
