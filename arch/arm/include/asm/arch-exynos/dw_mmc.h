/*
 * (C) Copyright 2011 SAMSUNG Electronics
 * Jaehoon Chung <jh80.chung@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __ASM_ARCH_DW_MMC_H_
#define __ASM_ARCH_DW_MMC_H_

#ifndef __ASSEMBLY__
struct s5p_dw_mmc {
	unsigned int	ctrl;
	unsigned int	pwren;
	unsigned int	clkdiv;
	unsigned int	clksrc;
	unsigned int	clkena;
	unsigned int	tmout;
	unsigned int	ctype;
	unsigned int	blksiz;
	unsigned int	bytcnt;
	unsigned int	intmask;
	unsigned int	cmdarg;
	unsigned int	cmd;
	unsigned int	resp0;
	unsigned int	resp1;
	unsigned int	resp2;
	unsigned int	resp3;
	unsigned int	mintsts;
	unsigned int	rintsts;
	unsigned int	status;
	unsigned int	fifo;
	unsigned int	cdetect;
	unsigned int	wrtprt;
	unsigned int	gpio;
	unsigned int	tcpcnt;
	unsigned int	tbbcnt;
	unsigned int	debnce;
	unsigned int	usrid;
	unsigned int	verid;
	unsigned int	hcon;
	unsigned int	uhs_reg;
	unsigned char	res1[8];
	unsigned int	bmod;
	unsigned int	pldmnd;
	unsigned int	dbaddr;
	unsigned int	idsts;
	unsigned int	idinten;
	unsigned int	dscaddr;
	unsigned int	bufaddr;
	unsigned int	clksel;
	unsigned int	res2[0xff5f];
};

struct dw_mmc_host {
	struct s5p_dw_mmc *reg;
	unsigned int clock;
	int dev_index;
};

int s5p_mmc_init(int dev_index, int bus_width);

#endif	/* __ASSEMBLY__ */
#endif  /* __ASM_ARCH_DW_MMC_H_ */
