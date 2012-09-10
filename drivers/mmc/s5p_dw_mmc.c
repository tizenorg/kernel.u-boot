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
 */

#include <common.h>
#include <mmc.h>
#include <asm/io.h>
#include <asm/arch/dw_mmc.h>
#include <asm/arch/mmc.h>
#include <asm/arch/clk.h>

struct mmc mmc_dev;
struct dw_mmc_host mmc_host;

static inline struct s5p_dw_mmc *s5p_get_base_mmc(int dev_index)
{
	/*
	 * FIXME: using MMC based offset 
	 * Based on MMC register offset 
	 **/
	unsigned long offset = 0x40000;
	return (struct s5p_dw_mmc *)(samsung_get_base_mmc() + offset);
}

static int dw_mmc_reset(struct dw_mmc_host *host)
{
	unsigned int timeout;
	unsigned int ctrl;

	ctrl = readl(&host->reg->ctrl);

	writel(ctrl | (1 << 2) | (1 << 1) | (1 << 0), &host->reg->ctrl);

	timeout = 1000;

	do {
		ctrl = readl(&host->reg->ctrl);
		if (!(ctrl & 0x7))
			return 1;
	} while (timeout--);

	return 0;
}

static void dw_mmc_prepare_data(struct dw_mmc_host *host,
		struct mmc_data *data)
{
	int timeout = 1000;
	unsigned int ctrl;

	writel((unsigned int) data->dest, &host->reg->dbaddr);

	writel((1 << 1), &host->reg->ctrl);

	/* reset */
	do {
		ctrl = readl(&host->reg->ctrl);
		if (!(ctrl & 0x2))
			break;
	} while (timeout--);

	ctrl = readl(&host->reg->ctrl);
	ctrl |= (1 << 25) | (1 << 5);
	writel(ctrl, &host->reg->ctrl);

	ctrl = readl(&host->reg->bmod);
	ctrl |= (1 << 7) | (1 << 1);
	writel(ctrl, &host->reg->bmod);

	writel(data->blocksize, &host->reg->blksiz);
	writel(data->blocks * data->blocksize, &host->reg->bytcnt);
}

static unsigned long dw_mmc_set_transfer_mode(struct dw_mmc_host *host,
		struct mmc_data *data)
{
	unsigned long mode;

	mode = (1 << 9);
	if (data->flags & MMC_DATA_WRITE)
		mode |= (1 << 10);

	return mode;
}

static int dw_mmc_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd,
		struct mmc_data *data)
{
	struct dw_mmc_host *host = (struct dw_mmc_host *)mmc->priv;
	int retry = 10000;
	unsigned int mask;
	unsigned long flags = 0;
	int i;

	writel(0xffff, &host->reg->rintsts);

	udelay(100);

	if (data)
		dw_mmc_prepare_data(host, data);

	writel(cmd->cmdarg, &host->reg->cmdarg);

	if (data) {
		flags = dw_mmc_set_transfer_mode(host, data);
	}

	if ((cmd->resp_type & MMC_RSP_136) && (cmd->resp_type & MMC_RSP_BUSY))
		return -1;

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		flags |= (1 << 6);
		if (cmd->resp_type & MMC_RSP_136) {
			flags |= (1 << 7);
		}
	}

	if (cmd->resp_type & MMC_RSP_CRC)
		flags |= (1 << 8);

	flags |= (cmd->cmdidx | (1 << 31) | (1 << 13));
	debug("cmd: %d\n", cmd->cmdidx);

	writel(flags, &host->reg->cmd);

	for (i = 0; i < retry; i++) {
		mask = readl(&host->reg->rintsts);
		if (mask & (1 << 2)) {
			if (!data)
				writel(mask, &host->reg->rintsts);
			break;
		}
	}

	if (i == retry) {
		printf("%s: waiting for interrupt status update\n", __func__);
		return TIMEOUT;
	}

	if (mask & (1 << 8)) {
		printf("response timeout: %08x cmd %d\n", mask, cmd->cmdidx);
		return TIMEOUT;
	} else if (mask & (1 << 1)) {
		printf("response error : %08x cmd %d\n", mask, cmd->cmdidx);
		return TIMEOUT;
	} else if (mask & (1 << 6)) {
		printf("response CRC error : %08x cmd %d\n", mask, cmd->cmdidx);
		return TIMEOUT;
	}

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136) {
			for (i = 0; i < 4; i++) {
				unsigned int offset =
					(unsigned int)(&host->reg->resp3 - i);
				cmd->response[i] = readl(offset);
				debug("cmd->resp[%d]: %08x\n",
						i, cmd->response[i]);
			}
		} else if (cmd->resp_type & MMC_RSP_BUSY) {
			retry = 0x10000;
			/* Card Busy check */
			for (i = 0; i < retry; i++) {
				if (!(readl(&host->reg->status)
					& (1 << 9)))
					break;
			}

			if (i == retry) {
				printf("%s: card is still busy\n", __func__);
				return TIMEOUT;
			}

			cmd->response[0] = readl(&host->reg->resp0);
			debug("cmd->resp[0]: %08x\n", __LINE__,cmd->response[0]);
		} else {
			cmd->response[0] = readl(&host->reg->resp0);
				debug("cmd->resp[%d]: %08x\n",
						i, cmd->response[i]);
		}
	}

	if (data) {
		while (1) {
			mask = readl(&host->reg->rintsts);
			if (mask & ((1 << 15) | (1 << 13) | (1 << 12) | (1 << 11) |
					(1 << 7) | (1 << 9) | (1 << 10))) {
				printf("%s: error during transfer: 0x%08x\n",
						__func__, mask);
				return -1;
			} else if (mask & (1 << 3))
				break;
			else if (mask & (1 << 2))
				break;
		}
		writel(mask, &host->reg->rintsts);
	}

	udelay(1000);

	return 0;
}

static int mci_send_cmd(struct dw_mmc_host *host)
{
	unsigned long timeout;

	writel(((1 << 31) | (1 << 21) | (1 << 13)), &host->reg->cmd);
	timeout = 10;
	while (readl(&host->reg->cmd) & (1 << 31)) {
		if (timeout == 0) {
			printf("%s: timeout error\n",__func__);
			return TIMEOUT;
		}
		timeout--;
		udelay(1000);
	}

	return 0;
}

static void dw_mmc_set_clock(struct dw_mmc_host *host, uint clock)
{
	int div;

	if (clock == 0)
		goto out;
	else if (clock <= 400000)
		div = 0x100;
	else if (clock <= 20000000)
		div = 4;
	else if (clock <= 26000000)
		div = 2;
	else
		div = 1;
	debug("div: %d\n", div);

	div >>= 1;

	writel(0, &host->reg->clkena);
	writel(0, &host->reg->clksrc);

	if (mci_send_cmd(host))
		return;

	writel(div, &host->reg->clkdiv);

	if (mci_send_cmd(host))
		return;

	writel((1 << 0), &host->reg->clkena);

	if (mci_send_cmd(host))
		return;

out:
	host->clock = clock;
}

/* DONE */
static void dw_mmc_set_ios(struct mmc *mmc)
{
	struct dw_mmc_host *host = mmc->priv;
	unsigned int ctrl;

	printf("bus_width: %x, clock: %d\n", mmc->bus_width, mmc->clock);

	dw_mmc_set_clock(host, mmc->clock);

	if (mmc->bus_width == 8) {
		ctrl = (1 << 16);
	} else if (mmc->bus_width == 4) {
		ctrl = (1 << 0);
	} else
		ctrl = (0 << 0);

	/*
	 * S.LSI guide this value at CLKSEL register
	 */
	writel(0x10001, &host->reg->clksel);

	writel(ctrl, &host->reg->ctype);
}

static int dw_mmc_core_init(struct mmc *mmc)
{
	struct dw_mmc_host *host = (struct dw_mmc_host *)mmc->priv;
	unsigned int ctrl;

	if (!dw_mmc_reset(host)) {
		printf("%s: dw_mmc reset timeout error!!!\n", __func__);
		return -1;
	}

	writel(0xffffffff, &host->reg->rintsts);
	writel(0, &host->reg->intmask);

	writel(0xfffff, &host->reg->debnce);

	writel((1 << 0), &host->reg->bmod);

	/* unused */
	writel(((0x2 << 28) | (0x7 << 16) | (0x8 << 0)), &host->reg->fifo);

	writel(0, &host->reg->idinten);
	writel(0xffffffff, &host->reg->tmout);

	ctrl = readl(&host->reg->bmod);
	ctrl |= (1 << 7) | (1 << 1);
	writel(ctrl, &host->reg->bmod);

	return 0;
}

static int s5p_dw_mmc_initialize(int dev_index, int bus_width)
{
	struct mmc *mmc;

	mmc = &mmc_dev;

	sprintf(mmc->name, "SAMSUNG DW_MMC");
	mmc->priv = &mmc_host;
	mmc->send_cmd = dw_mmc_send_cmd;
	mmc->set_ios = dw_mmc_set_ios;
	mmc->init = dw_mmc_core_init;

	mmc->voltages = MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195;

	mmc->host_caps = MMC_MODE_8BIT | MMC_MODE_HS_52MHz | MMC_MODE_HS;

	mmc->f_min = 400000;
	mmc->f_max = 52000000;

	mmc_host.dev_index = dev_index;
	mmc_host.clock = 0;
	mmc_host.reg = s5p_get_base_mmc(dev_index);

	mmc_register(mmc);
	mmc->block_dev.dev = dev_index;

	return 0;
}

int s5p_mmc_init(int dev_index, int bus_width)
{
	return s5p_dw_mmc_initialize(dev_index, bus_width);
}
