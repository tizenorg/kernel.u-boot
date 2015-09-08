/*
 * Copyright (C) 2009 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 *
 * based on s3c24x0_i2c.c
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

#include <common.h>
#include <i2c.h>
#include <asm/io.h>
#include <asm/arch/i2c.h>
#include <asm/arch/clk.h>
#include <asm/arch/gpio.h>
#include <asm/arch/cpu.h>

#ifdef CONFIG_HARD_I2C

#define	I2C_WRITE	0
#define I2C_READ	1

#define I2C_OK		0
#define I2C_NOK		1
#define I2C_NACK	2
#define I2C_NOK_LA	3		/* Lost arbitration */
#define I2C_NOK_TOUT	4		/* time out */

#define I2CSTAT_BSY	0x20		/* Busy bit */
#define I2CSTAT_NACK	0x01		/* Nack bit */
#define I2CCON_IRPND	0x10		/* Interrupt pending bit */
#define I2C_MODE_MT	0xC0		/* Master Transmit Mode */
#define I2C_MODE_MR	0x80		/* Master Receive Mode */
#define I2C_START_STOP	0x20		/* START / STOP */
#define I2C_TXRX_ENA	0x10		/* I2C Tx/Rx enable */

#define I2C_TIMEOUT	1		/* 1 second */

static unsigned int default_channel;

static inline s5pc1xx_i2c_t *s5pc1xx_get_base_i2c(void)
{
	if (cpu_is_s5pc110()) {
		default_channel = 2;
		return (s5pc1xx_i2c_t *) S5PC110_I2C2_BASE;
	}

	default_channel = 0;
	return (s5pc1xx_i2c_t *) S5PC100_I2C0_BASE;
}

static int WaitForXfer(void)
{
	s5pc1xx_i2c_t *const i2c = s5pc1xx_get_base_i2c();
	int i;
	int status;

	i = I2C_TIMEOUT * 10000;
	status = i2c->IICCON;

	while ((i > 0) && !(status & I2CCON_IRPND)) {
		udelay(100);
		status = i2c->IICCON;
		i--;
	}

	return (status & I2CCON_IRPND) ? I2C_OK : I2C_NOK_TOUT;
}

static int IsACK(void)
{
	s5pc1xx_i2c_t *i2c = s5pc1xx_get_base_i2c();

	return !(i2c->IICSTAT & I2CSTAT_NACK);
}

static void ReadWriteByte(void)
{
	s5pc1xx_i2c_t *i2c = s5pc1xx_get_base_i2c();

	i2c->IICCON &= ~I2CCON_IRPND;
}

void i2c_init(int speed, int slaveadd)
{
	s5pc1xx_i2c_t *i2c = s5pc1xx_get_base_i2c();
	u32 freq;
	u32 pres = 16;
	u32 div;
	int i;
	int status, value, reg;
	int gpio_shift = 0;

	/* wait for some time to give previous transfer a chance to finish */
	i = I2C_TIMEOUT * 1000;
	status = i2c->IICSTAT;

	while ((i > 0) && (status & I2CSTAT_BSY)) {
		udelay(1000);
		status = i2c->IICSTAT;
		i--;
	}

	/* calculate prescaler and divisor values */
	freq = get_pclk();

	if (((freq >> 4) / speed) > 0xf) {
		pres = 1;
		div = (freq >> 9) / speed;	/* PCLK/512/freq */
	} else {
		pres = 0;
		div = (freq >> 4) / speed;	/* PCLK/16/freq */
	}
	/* set prescaler, divisor according to freq, also set
	 * ACKGEN, IRQ */
	i2c->IICCON = (pres << 6) | (1 << 5) | (div & 0xf);

	/* init to SLAVE REVEIVE and set slaveaddr */
	i2c->IICSTAT = 0;
	i2c->IICADD = slaveadd;
	/* program Master Transmit (and implicit STOP) */
	i2c->IICSTAT = I2C_MODE_MT | I2C_TXRX_ENA;

	if (cpu_is_s5pc110()) {
		/* set I2C pad conf */
		switch (default_channel) {
		case 2:
			gpio_shift = 16;
			break;
		case 1:
			gpio_shift = 8;
			break;
		case 0:
			gpio_shift = 0;
			break;
		default:
			break;
		}

		reg = S5PC110_GPIO_BASE(S5PC110_GPIO_D1_OFFSET);
		reg += S5PC1XX_GPIO_CON_OFFSET;
	} else {
		/* set I2C pad conf */
		if (default_channel)
			gpio_shift = 20;
		else
			gpio_shift = 12;

		reg = S5PC100_GPIO_BASE(S5PC100_GPIO_D_OFFSET);
		reg += S5PC1XX_GPIO_CON_OFFSET;
	}
	value = readl(reg);
	value &= ~(0xff << gpio_shift);
	value |= (0x22 << gpio_shift);
	writel(value, reg);
}

/*
 * cmd_type is 0 for write, 1 for read.
 *
 * addr_len can take any value from 0-255, it is only limited
 * by the char, we could make it larger if needed. If it is
 * 0 we skip the address write cycle.
 */
static
int i2c_transfer(unsigned char cmd_type, unsigned char chip,
		unsigned char addr[], unsigned char addr_len,
		unsigned char data[], unsigned short data_len)
{
	s5pc1xx_i2c_t *i2c = s5pc1xx_get_base_i2c();
	int i;
	int status;
	int result;

	if (data == 0 || data_len == 0) {
		/*Don't support data transfer of no length or to address 0 */
		printf("i2c_transfer: bad call\n");
		return I2C_NOK;
	}

	/* Check I2C bus idle */
	i = I2C_TIMEOUT * 1000;
	status = i2c->IICSTAT;

	while ((i > 0) && (status & I2CSTAT_BSY)) {
		udelay(1000);
		status = i2c->IICSTAT;
		i--;
	}

	if (status & I2CSTAT_BSY)
		return I2C_NOK_TOUT;

	i2c->IICCON |= 0x80;
	result = I2C_OK;

	switch (cmd_type) {
	case I2C_WRITE:
		if (addr && addr_len) {
			i2c->IICDS = chip;
			/* send START */
			i2c->IICSTAT = I2C_MODE_MT | I2C_TXRX_ENA | I2C_START_STOP;
			i = 0;
			while ((i < addr_len) && (result == I2C_OK)) {
				result = WaitForXfer();
				i2c->IICDS = addr[i];
				ReadWriteByte();
				i++;
			}
			i = 0;
			while ((i < data_len) && (result == I2C_OK)) {
				result = WaitForXfer();
				i2c->IICDS = data[i];
				ReadWriteByte();
				i++;
			}
		} else {
			i2c->IICDS = chip;
			/* send START */
			i2c->IICSTAT = I2C_MODE_MT | I2C_TXRX_ENA | I2C_START_STOP;
			i = 0;
			while ((i < data_len) && (result = I2C_OK)) {
				result = WaitForXfer();
				i2c->IICDS = data[i];
				ReadWriteByte();
				i++;
			}
		}

		if (result == I2C_OK)
			result = WaitForXfer();

		/* send STOP */
		i2c->IICSTAT = I2C_MODE_MR | I2C_TXRX_ENA;
		ReadWriteByte();
		break;

	case I2C_READ:
		if (addr && addr_len) {
			i2c->IICSTAT = I2C_MODE_MT | I2C_TXRX_ENA;
			i2c->IICDS = chip;

			/* send START */
			i2c->IICSTAT |= I2C_START_STOP;
			result = WaitForXfer();
			if (IsACK()) {
				i = 0;
				while ((i < addr_len) && (result == I2C_OK)) {
					i2c->IICDS = addr[i];
					ReadWriteByte();
					result = WaitForXfer();
					i++;
				}

				i2c->IICDS = chip;
				/* resend START */
				i2c->IICSTAT =  I2C_MODE_MR | I2C_TXRX_ENA |
						I2C_START_STOP;
				ReadWriteByte();
				result = WaitForXfer();
				i = 0;
				while ((i < data_len) && (result == I2C_OK)) {
					/* disable ACK for final READ */
					if (i == data_len - 1)
						i2c->IICCON &= ~0x80;
					ReadWriteByte();
					result = WaitForXfer();
					data[i] = i2c->IICDS;
					i++;
				}
			} else {
				result = I2C_NACK;
			}

		} else {
			i2c->IICSTAT = I2C_MODE_MR | I2C_TXRX_ENA;
			i2c->IICDS = chip;
			/* send START */
			i2c->IICSTAT |= I2C_START_STOP;
			result = WaitForXfer();

			if (IsACK()) {
				i = 0;
				while ((i < data_len) && (result == I2C_OK)) {
					/* disable ACK for final READ */
					if (i == data_len - 1)
						i2c->IICCON &= ~0x80;
					ReadWriteByte();
					result = WaitForXfer();
					data[i] = i2c->IICDS;
					i++;
				}
			} else {
				result = I2C_NACK;
			}
		}

		/* send STOP */
		i2c->IICSTAT = I2C_MODE_MR | I2C_TXRX_ENA;
		ReadWriteByte();
		break;

	default:
		printf("i2c_transfer: bad call\n");
		result = I2C_NOK;
		break;
	}

	return result;
}

int i2c_probe(uchar chip)
{
	uchar buf[1];

	buf[0] = 0;

	/*
	 * What is needed is to send the chip address and verify that the
	 * address was <ACK>ed (i.e. there was a chip at that address which
	 * drove the data line low).
	 */
	return (i2c_transfer(I2C_READ, chip << 1, 0, 0, buf, 1) != I2C_OK);
}

int i2c_read(uchar chip, uint addr, int alen, uchar *buffer, int len)
{
	uchar xaddr[4];
	int ret;

	if (alen > 4) {
		printf("I2C read: addr len %d not supported\n", alen);
		return 1;
	}

	if (alen > 0) {
		xaddr[0] = (addr >> 24) & 0xFF;
		xaddr[1] = (addr >> 16) & 0xFF;
		xaddr[2] = (addr >> 8) & 0xFF;
		xaddr[3] = addr & 0xFF;
	}

#ifdef CFG_I2C_EEPROM_ADDR_OVERFLOW
	/*
	 * EEPROM chips that implement "address overflow" are ones
	 * like Catalyst 24WC04/08/16 which has 9/10/11 bits of
	 * address and the extra bits end up in the "chip address"
	 * bit slots. This makes a 24WC08 (1Kbyte) chip look like
	 * four 256 byte chips.
	 *
	 * Note that we consider the length of the address field to
	 * still be one byte because the extra address bits are
	 * hidden in the chip address.
	 */
	if (alen > 0)
		chip |= ((addr >> (alen * 8)) & CFG_I2C_EEPROM_ADDR_OVERFLOW);
#endif
	ret = i2c_transfer(I2C_READ, chip << 1, &xaddr[4 - alen], alen, buffer, len);
	if (ret != 0) {
		printf("I2c read: failed %d\n", ret);
		printf("%x%x\n", buffer[1], buffer[0]);
		return 1;
	}
	return 0;
}

int i2c_write(uchar chip, uint addr, int alen, uchar *buffer, int len)
{
	uchar xaddr[4];

	if (alen > 4) {
		printf("I2C write: addr len %d not supported\n", alen);
		return 1;
	}

	if (alen > 0) {
		xaddr[0] = (addr >> 24) & 0xFF;
		xaddr[1] = (addr >> 16) & 0xFF;
		xaddr[2] = (addr >> 8) & 0xFF;
		xaddr[3] = addr & 0xFF;
	}
#ifdef CONFIG_SYS_I2C_EEPROM_ADDR_OVERFLOW
	/*
	 * EEPROM chips that implement "address overflow" are ones
	 * like Catalyst 24WC04/08/16 which has 9/10/11 bits of
	 * address and the extra bits end up in the "chip address"
	 * bit slots. This makes a 24WC08 (1Kbyte) chip look like
	 * four 256 byte chips.
	 *
	 * Note that we consider the length of the address field to
	 * still be one byte because the extra address bits are
	 * hidden in the chip address.
	 */
	if (alen > 0)
		chip |= ((addr >> (alen * 8)) & CONFIG_SYS_I2C_EEPROM_ADDR_OVERFLOW);
#endif
	return (i2c_transfer(I2C_WRITE, chip << 1, &xaddr[4 - alen], alen,
				buffer, len) != 0);
}
#endif	/* CONFIG_HARD_I2C */
