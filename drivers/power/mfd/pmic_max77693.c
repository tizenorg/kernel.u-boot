/*
 *  Copyright (C) 2013 Samsung Electronics
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
#include <power/max77693_pmic.h>
#include <i2c.h>
#include <errno.h>

static int max77693_charger_state(struct pmic *p, int state, int current)
{
	unsigned int val;

	if (pmic_probe(p))
		return -1;

	/* unlock write capability */
	val = MAX77693_CHG_UNLOCK;
	pmic_reg_write(p, MAX77693_CHG_CNFG_06, val);

	if (state == CHARGER_DISABLE) {
		puts("Disable the charger.\n");
		pmic_reg_read(p, MAX77693_CHG_CNFG_00, &val);
		val &= ~0x01;
		pmic_reg_write(p, MAX77693_CHG_CNFG_00, val);
		return -1;
	}

	if (current < CHARGER_MIN_CURRENT || current > CHARGER_MAX_CURRENT) {
		printf("%s: Wrong charge current: %d [mA]\n",
		       __func__, current);
		return -1;
	}

	/* set charging current */
	pmic_reg_read(p, MAX77693_CHG_CNFG_02, &val);
	val &= ~MAX77693_CHG_CC;
	val |= current * 10 / 333;	/* 0.1A/3 steps */
	pmic_reg_write(p, MAX77693_CHG_CNFG_02, val);

	/* enable charging */
	val = MAX77693_CHG_MODE_ON;
	pmic_reg_write(p, MAX77693_CHG_CNFG_00, val);

	/* check charging current */
	pmic_reg_read(p, MAX77693_CHG_CNFG_02, &val);
	val &= 0x3f;
	printf("Enable the charger @ %d [mA]\n", val * 333 / 10);

	return 0;
}

static int max77693_charger_bat_present(struct pmic *p)
{
	unsigned int val;

	if (pmic_probe(p))
		return -1;

	pmic_reg_read(p, MAX77693_CHG_INT_OK, &val);

	return !(val & MAX77693_CHG_DETBAT);
}

static struct power_chrg power_chrg_pmic_ops = {
	.chrg_bat_present = max77693_charger_bat_present,
	.chrg_state = max77693_charger_state,
};

int pmic_init_max77693(unsigned char bus)
{
	static const char name[] = "MAX77693_PMIC";
	struct pmic *p = pmic_alloc();

	if (!p) {
		printf("%s: POWER allocation error!\n", __func__);
		return -ENOMEM;
	}

	debug("Board PMIC init\n");

	p->name = name;
	p->interface = PMIC_I2C;
	p->number_of_regs = PMIC_NUM_OF_REGS;
	p->hw.i2c.addr = MAX77693_PMIC_I2C_ADDR;
	p->hw.i2c.tx_num = 1;
	p->bus = bus;

	p->chrg = &power_chrg_pmic_ops;
	return 0;
}
