/*
 * Copyright (C) 2010 Samsung Electronics
 * spi_gpio.c - SPI master driver using generic bitbanged GPIO
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
#include <asm/arch/gpio.h>
#include <spi.h>

static void spi_gpio_set_sck(struct spi_platform_data *spi, int is_on)
{
	gpio_set_value(spi->clk_bank, spi->clk_num, is_on);
}

static void spi_gpio_set_mosi(struct spi_platform_data *spi, int is_on)
{
	gpio_set_value(spi->si_bank, spi->si_num, is_on);
}

static void spi_gpio_chipselect(struct spi_platform_data *spi,
	int cpol)
{
	gpio_set_value(spi->cs_bank, spi->cs_num,
		!spi->cs_active);

	/* set initial clock polarity */
	if (cpol)
		spi_gpio_set_sck(spi, spi->mode & SPI_CPOL);

	/* SPI is normally active-low */
	gpio_set_value(spi->cs_bank, spi->cs_num,
		spi->cs_active);
}

static void
spi_gpio_tx_cpha0(struct spi_platform_data *spi,
		unsigned int nsecs, unsigned int cpol,
		unsigned int word, unsigned int bits)
{
	int i;
	unsigned int data;

	data = (word << spi->word_len) + bits;

	spi_gpio_chipselect(spi, cpol);

	/* clock starts at inactive polarity */
	for (i = spi->word_len; i >= 0; i--) {

		/* data high or low */
		if ((data >> i) & 0x1)
			spi_gpio_set_mosi(spi, 1);
		else
			spi_gpio_set_mosi(spi, 0);

		udelay(nsecs);

		spi_gpio_set_sck(spi, !cpol);
		udelay(nsecs);

		spi_gpio_set_sck(spi, cpol);
	}
}

static void
spi_gpio_tx_cpha1(struct spi_platform_data *spi,
		unsigned int nsecs, unsigned int cpol,
		unsigned int word, unsigned int bits)
{
	int i;
	unsigned int data;

	data = (word << spi->word_len) + bits;

	spi_gpio_chipselect(spi, cpol);

	/* clock starts at inactive polarity */
	for (i = spi->word_len; i >= 0; i--) {

		/* setup MSB (to slave) on leading edge */
		spi_gpio_set_sck(spi, !cpol);

		/* data high or low */
		if ((data >> i) & 0x1)
			spi_gpio_set_mosi(spi, 1);
		else
			spi_gpio_set_mosi(spi, 0);

		udelay(nsecs);

		spi_gpio_set_sck(spi, cpol);
		udelay(nsecs);
	}
}

static void spi_gpio_tx_word_mode0(struct spi_platform_data *spi,
		unsigned int nsecs, unsigned int word, unsigned int bits)
{
	return spi_gpio_tx_cpha0(spi, nsecs, 0, word, bits);
}

static void spi_gpio_tx_word_mode1(struct spi_platform_data *spi,
		unsigned int nsecs, unsigned int word, unsigned int bits)
{
	return spi_gpio_tx_cpha1(spi, nsecs, 0, word, bits);
}

static void spi_gpio_tx_word_mode2(struct spi_platform_data *spi,
		unsigned int nsecs, unsigned int word, unsigned int bits)
{
	return spi_gpio_tx_cpha0(spi, nsecs, 1, word, bits);
}

static void spi_gpio_tx_word_mode3(struct spi_platform_data *spi,
		unsigned int nsecs, unsigned int word, unsigned int bits)
{
	return spi_gpio_tx_cpha1(spi, nsecs, 1, word, bits);
}

void spi_gpio_write(struct spi_platform_data *spi,
	unsigned int address, unsigned int command)
{
	switch (spi->mode) {
	case SPI_MODE_0:
		spi_gpio_tx_word_mode0(spi,
			1, address, command);
	case SPI_MODE_1:
		spi_gpio_tx_word_mode1(spi,
			1, address, command);
	case SPI_MODE_2:
		spi_gpio_tx_word_mode2(spi,
			1, address, command);
	case SPI_MODE_3:
		spi_gpio_tx_word_mode3(spi,
			1, address, command);
	}
}
