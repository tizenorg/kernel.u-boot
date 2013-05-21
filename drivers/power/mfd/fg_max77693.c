/*
 *  Copyright (C) 2013 Samsung Electronics
 *  Piotr Wilczek <p.wilczek@samsung.com>
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
#include <power/pmic.h>
#include <power/max77693_fg.h>
#include <i2c.h>
#include <power/power_chrg.h>
#include <power/battery.h>
#include <power/fg_battery_cell_params.h>
#include <errno.h>

static int max77693_get_vcell(u32 *vcell)
{
	u16 value;
	u8 ret;

	ret = i2c_read(MAX77693_FUEL_I2C_ADDR, MAX77693_VCELL, 1,
		       (u8 *)&value, 2);
	if (ret)
		return -1;

	*vcell = (u32)(value >> 3);
	*vcell = *vcell * 625;

	return 0;
}

static int max77693_get_soc(u32 *soc)
{
	u16 value;
	u8 ret;

	ret = i2c_read(MAX77693_FUEL_I2C_ADDR, MAX77693_VFSOC, 1,
		       (u8 *)&value, 2);
	if (ret)
		return -1;

	*soc = (u32)(value >> 8);

	return 0;
}

static int power_update_battery(struct pmic *p, struct pmic *bat)
{
	struct power_battery *pb = bat->pbat;
	int ret = 0;

	if (pmic_probe(p)) {
		puts("Can't find max17042 fuel gauge\n");
		return -1;
	}

	max77693_get_soc(&pb->bat->state_of_chrg);
	max77693_get_vcell(&pb->bat->voltage_uV);

	return ret;
}

static int power_check_battery(struct pmic *p, struct pmic *bat)
{
	struct power_battery *pb = bat->pbat;
	unsigned int val;
	int ret = 0;

	if (pmic_probe(p)) {
		puts("Can't find max17042 fuel gauge\n");
		return -1;
	}

	ret |= pmic_reg_read(p, MAX77693_STATUS, &val);
	debug("fg status: 0x%x\n", val);

	ret |= pmic_reg_read(p, MAX77693_VERSION, &pb->bat->version);

	power_update_battery(p, bat);
	debug("fg ver: 0x%x\n", pb->bat->version);
	printf("BAT: state_of_charge(SOC):%d%%\n",
	       pb->bat->state_of_chrg);

	printf("     voltage: %d.%6.6d [V] (expected to be %d [mAh])\n",
	       pb->bat->voltage_uV / 1000000,
	       pb->bat->voltage_uV % 1000000,
	       pb->bat->capacity);

	if (pb->bat->voltage_uV > 3850000)
		pb->bat->state = EXT_SOURCE;
	else if (pb->bat->voltage_uV < 3600000 || pb->bat->state_of_chrg < 5)
		pb->bat->state = CHARGE;
	else
		pb->bat->state = NORMAL;

	return ret;
}

static struct power_fg power_fg_ops = {
	.fg_battery_check = power_check_battery,
	.fg_battery_update = power_update_battery,
};

int power_fg_init(unsigned char bus)
{
	static const char name[] = "MAX77693_FG";
	struct pmic *p = pmic_alloc();

	if (!p) {
		printf("%s: POWER allocation error!\n", __func__);
		return -ENOMEM;
	}

	debug("Board Fuel Gauge init\n");

	p->name = name;
	p->interface = PMIC_I2C;
	p->number_of_regs = FG_NUM_OF_REGS;
	p->hw.i2c.addr = MAX77693_FUEL_I2C_ADDR;
	p->hw.i2c.tx_num = 2;
	p->sensor_byte_order = PMIC_SENSOR_BYTE_ORDER_BIG;
	p->bus = bus;

	p->fg = &power_fg_ops;
	return 0;
}
