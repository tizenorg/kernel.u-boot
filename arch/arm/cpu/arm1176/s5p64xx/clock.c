/*
 * Copyright (C) 2009 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
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

#include <common.h>
#include <asm/io.h>
#include <asm/arch/clock.h>

#ifndef CONFIG_SYS_CLK_FREQ_6442
#define CONFIG_SYS_CLK_FREQ_6442	12000000
#endif

unsigned long (*get_pclk)(void);
unsigned long (*get_arm_clk)(void);
unsigned long (*get_pll_clk)(int);

/* s5p6442: return pll clock frequency */
static unsigned long s5p6442_get_pll_clk(int pllreg)
{
	struct s5p6442_clock *clk = (struct s5p6442_clock *)S5P64XX_CLOCK_BASE;
	unsigned long r, m, p, s, mask, fout;
	unsigned int freq;

	switch (pllreg) {
	case APLL:
		r = readl(&clk->apll_con);
		break;
	case MPLL:
		r = readl(&clk->mpll_con);
		break;
	case EPLL:
		r = readl(&clk->epll_con);
		break;
	case VPLL:
		r = readl(&clk->vpll_con);
		break;
	default:
		printf("Unsupported PLL (%d)\n", pllreg);
		return 0;
	}

	/*
	 * APLL_CON: MIDV [25:16]
	 * MPLL_CON: MIDV [25:16]
	 * EPLL_CON: MIDV [24:16]
	 * VPLL_CON: MIDV [24:16]
	 */
	if (pllreg == APLL || pllreg == MPLL)
		mask = 0x3ff;
	else
		mask = 0x1ff;

	m = (r >> 16) & mask;

	/* PDIV [13:8] */
	p = (r >> 8) & 0x3f;
	/* SDIV [2:0] */
	s = r & 0x7;

	freq = CONFIG_SYS_CLK_FREQ_6442;
	if (pllreg == APLL) {
		if (s < 1)
			s = 1;
		/* FOUT = MDIV * FIN / (PDIV * 2^(SDIV - 1)) */
		fout = m * (freq / (p * (1 << (s - 1))));
	} else
		/* FOUT = MDIV * FIN / (PDIV * 2^SDIV) */
		fout = m * (freq / (p * (1 << s)));

	return fout;
}

/* s5p6442: return ARM clock frequency */
static unsigned long s5p6442_get_arm_clk(void)
{
	struct s5p6442_clock *clk = (struct s5p6442_clock *)S5P64XX_CLOCK_BASE;
	unsigned long div;
	unsigned long dout_apll, armclk;
	unsigned int apll_ratio;

	div = readl(&clk->div0);

	/* APLL_RATIO: [2:0] */
	apll_ratio = div & 0x7;

	dout_apll = get_pll_clk(APLL) / (apll_ratio + 1);
	armclk = dout_apll;

	return armclk;
}

/* s5p6442: return D0CLK frequency */
static unsigned long get_d0clk(void)
{
	struct s5p6442_clock *clk = (struct s5p6442_clock *)S5P64XX_CLOCK_BASE;
	unsigned long d0clk;
	uint div, d0_bus_ratio;

	div = readl(&clk->div0);
	/* D0CLK_RATIO: [19:16] */
	d0_bus_ratio = (div >> 16) & 0xf;

	d0clk = get_arm_clk() / (d0_bus_ratio + 1);

	return d0clk;
}

/* s5p6442: return P1CLK frequency */
static unsigned long get_p1clk(void)
{
	struct s5p6442_clock *clk = (struct s5p6442_clock *)S5P64XX_CLOCK_BASE;
	unsigned long d1_bus, p1clk;
	uint div, d1_bus_ratio, p1clk_ratio;

	div = readl(&clk->div0);
	/* D1CLK_RATIO: [27:24] */
	d1_bus_ratio = (div >> 24) & 0xf;
	/* P1CLK_RATIO: [30:28] */
	p1clk_ratio = (div >> 28) & 0x7;

	/* ASYNC Mode */
	d1_bus = get_pll_clk(MPLL) / (d1_bus_ratio + 1);
	p1clk = d1_bus / (p1clk_ratio + 1);

	return p1clk;
}

/* s5p6442: return peripheral clock frequency */
static unsigned long s5p6442_get_pclk(void)
{
	return get_p1clk();
}

void s5p64xx_clock_init(void)
{
	if (cpu_is_s5p6442()) {
		get_pll_clk = s5p6442_get_pll_clk;
		get_arm_clk = s5p6442_get_arm_clk;
		get_pclk = s5p6442_get_pclk;
	}
}
