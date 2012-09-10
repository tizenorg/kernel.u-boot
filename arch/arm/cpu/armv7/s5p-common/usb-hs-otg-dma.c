/*
 * Copyright (C) 2009 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
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

#include <command.h>
#include <asm/errno.h>
#include <asm/arch/cpu.h>
#include <usbd.h>
#include "usb-hs-otg.h"

static u32 remode_wakeup;
static u16 config_value __attribute__((aligned(8)));
static u8 zlp_buf __attribute__((aligned(8)));

int s5p_receive_done;
int s5p_usb_connected;

#ifdef CONFIG_S5P_USB_DMA
static USB_OPMODE op_mode = USB_DMA;
#else
static USB_OPMODE op_mode = USB_CPU;
#endif
static USB_SPEED speed = USB_HIGH;
/*
USB_OPMODE op_mode = USB_DMA;
USB_SPEED speed = USB_FULL;
*/

otg_dev_t otg;
static get_status_t get_status;
static get_intf_t get_intf;

enum EP_INDEX {
	EP0, EP1, EP2, EP3, EP4
};

/*------------------------------------------------*/
/* EP0 state */
enum EP0_STATE {
	EP0_STATE_INIT = 0,
	EP0_STATE_GD_DEV_0 = 11,
	EP0_STATE_GD_DEV_1 = 12,
	EP0_STATE_GD_DEV_2 = 13,
	EP0_STATE_GD_CFG_0 = 21,
	EP0_STATE_GD_CFG_1 = 22,
	EP0_STATE_GD_CFG_2 = 23,
	EP0_STATE_GD_CFG_3 = 24,
	EP0_STATE_GD_CFG_4 = 25,
	EP0_STATE_GD_STR_I0 = 29,
	EP0_STATE_GD_STR_I1 = 30,
	EP0_STATE_GD_STR_I2 = 31,
	EP0_STATE_GD_STR_I3 = 32,
	EP0_STATE_GD_DEV_QUALIFIER = 33,
	EP0_STATE_INTERFACE_GET = 34,
	EP0_STATE_GET_STATUS0 = 35,
	EP0_STATE_GET_STATUS1 = 36,
	EP0_STATE_GET_STATUS2 = 37,
	EP0_STATE_GET_STATUS3 = 38,
	EP0_STATE_GET_STATUS4 = 39,
	EP0_STATE_GD_OTHER_SPEED = 40,
	EP0_STATE_GD_CFG_ONLY_0 = 41,
	EP0_STATE_GD_CFG_ONLY_1 = 42,
	EP0_STATE_GD_IF_ONLY_0 = 44,
	EP0_STATE_GD_IF_ONLY_1 = 45,
	EP0_STATE_GD_EP0_ONLY_0 = 46,
	EP0_STATE_GD_EP1_ONLY_0 = 47,
	EP0_STATE_GD_EP2_ONLY_0 = 48,
	EP0_STATE_GD_EP3_ONLY_0 = 49,
	EP0_STATE_GD_OTHER_SPEED_HIGH_1 = 51,
	EP0_STATE_GD_OTHER_SPEED_HIGH_2 = 52,
	EP0_STATE_GD_OTHER_SPEED_HIGH_3 = 53
};

/*definitions related to CSR setting */

/* OTG_GOTGCTL*/
#define B_SESSION_VALID		(0x1 << 19)
#define A_SESSION_VALID		(0x1 << 18)

/* OTG_GAHBCFG*/
#define PTXFE_HALF		(0<<8)
#define PTXFE_ZERO		(1<<8)
#define NPTXFE_HALF		(0<<7)
#define NPTXFE_ZERO		(1<<7)
#define MODE_SLAVE		(0<<5)
#define MODE_DMA		(1<<5)
#define BURST_SINGLE		(0<<1)
#define BURST_INCR		(1<<1)
#define BURST_INCR4		(3<<1)
#define BURST_INCR8		(5<<1)
#define BURST_INCR16		(7<<1)
#define GBL_INT_UNMASK		(1<<0)
#define GBL_INT_MASK		(0<<0)

/* OTG_GRSTCTL*/
#define AHB_MASTER_IDLE		(1u<<31)
#define CORE_SOFT_RESET		(0x1<<0)

/* OTG_GINTSTS/OTG_GINTMSK core interrupt register */
#define INT_RESUME		(1u<<31)
#define INT_DISCONN		(0x1<<29)
#define INT_CONN_ID_STS_CNG	(0x1<<28)
#define INT_OUT_EP		(0x1<<19)
#define INT_IN_EP		(0x1<<18)
#define INT_ENUMDONE		(0x1<<13)
#define INT_RESET		(0x1<<12)
#define INT_SUSPEND		(0x1<<11)
#define INT_TX_FIFO_EMPTY	(0x1<<5)
#define INT_RX_FIFO_NOT_EMPTY	(0x1<<4)
#define INT_SOF			(0x1<<3)
#define INT_DEV_MODE		(0x0<<0)
#define INT_HOST_MODE		(0x1<<1)
#define INT_OTG			(0x1<<2)

/* OTG_GRXSTSP STATUS*/
#define GLOBAL_OUT_NAK			(0x1<<17)
#define OUT_PKT_RECEIVED		(0x2<<17)
#define OUT_TRNASFER_COMPLETED		(0x3<<17)
#define SETUP_TRANSACTION_COMPLETED	(0x4<<17)
#define SETUP_PKT_RECEIVED		(0x6<<17)

/* OTG_DCTL device control register */
#define NORMAL_OPERATION		(0x1<<0)
#define SOFT_DISCONNECT			(0x1<<1)
#define	TEST_J_MODE			(TEST_J<<4)
#define	TEST_K_MODE			(TEST_K<<4)
#define	TEST_SE0_NAK_MODE		(TEST_SE0_NAK<<4)
#define	TEST_PACKET_MODE		(TEST_PACKET<<4)
#define	TEST_FORCE_ENABLE_MODE		(TEST_FORCE_ENABLE<<4)
#define TEST_CONTROL_FIELD		(0x7<<4)

/* OTG_DAINT device all endpoint interrupt register */
#define INT_IN_EP0			(0x1<<0)
#define INT_IN_EP1			(0x1<<1)
#define INT_IN_EP3			(0x1<<3)
#define INT_OUT_EP0			(0x1<<16)
#define INT_OUT_EP2			(0x1<<18)
#define INT_OUT_EP4			(0x1<<20)

/* OTG_DIEPCTL0/OTG_DOEPCTL0 */
#define DEPCTL_EPENA			(0x1<<31)
#define DEPCTL_EPDIS			(0x1<<30)
#define DEPCTL_SNAK			(0x1<<27)
#define DEPCTL_CNAK			(0x1<<26)
#define DEPCTL_CTRL_TYPE		(EP_TYPE_CONTROL<<18)
#define DEPCTL_ISO_TYPE			(EP_TYPE_ISOCHRONOUS<<18)
#define DEPCTL_BULK_TYPE		(EP_TYPE_BULK<<18)
#define DEPCTL_INTR_TYPE		(EP_TYPE_INTERRUPT<<18)
#define DEPCTL_USBACTEP			(0x1<<15)

/*ep0 enable, clear nak, next ep0, max 64byte */
#define EPEN_CNAK_EP0_64 (DEPCTL_EPENA|DEPCTL_CNAK|(CONTROL_EP<<11)|(0<<0))

/*ep0 enable, clear nak, next ep0, 8byte */
#define EPEN_CNAK_EP0_8 (DEPCTL_EPENA|DEPCTL_CNAK|(CONTROL_EP<<11)|(3<<0))

/* DIEPCTLn/DOEPCTLn */
#define BACK2BACK_SETUP_RECEIVED	(0x1<<6)
#define INTKN_TXFEMP			(0x1<<4)
#define NON_ISO_IN_EP_TIMEOUT		(0x1<<3)
#define CTRL_OUT_EP_SETUP_PHASE_DONE	(0x1<<3)
#define AHB_ERROR			(0x1<<2)
#define TRANSFER_DONE			(0x1<<0)

/* codes representing languages */
static const u8 string_desc0[] = {
	4, STRING_DESCRIPTOR, LANGID_US_L, LANGID_US_H,
};

static const u8 string_desc1[] =	/* Manufacturer */
{
	(0x1A + 2), STRING_DESCRIPTOR,
	'G', 0x0, 'a', 0x0, 'd', 0x0, 'g', 0x0, 'e', 0x0,
	't', 0x0, ' ', 0x0, 'S', 0x0, 'e', 0x0, 'r', 0x0,
	'i', 0x0, 'a', 0x0, 'l', 0x0
};

static const u8 string_desc2[] =	/* Product */
{
	(0x24 + 2), STRING_DESCRIPTOR,
	'S', 0x0, 'A', 0x0, 'M', 0x0, 'S', 0x0, 'U', 0x0,
	'N', 0x0, 'G', 0x0, ' ', 0x0, 'S', 0x0, 'L', 0x0,
	'P', 0x0, ' ', 0x0, 'D', 0x0, 'R', 0x0, 'I', 0x0,
	'V', 0x0, 'E', 0x0, 'R', 0x0
};

const u8 string_desc3[] =		/* Gadget Serial Control */
{
	(0x2a+2), STRING_DESCRIPTOR,
	'G', 0x0, 'a', 0x0, 'd', 0x0, 'g', 0x0, 'e', 0x0,
	't', 0x0, ' ', 0x0, 'S', 0x0, 'e', 0x0, 'r', 0x0,
	'i', 0x0, 'a', 0x0, 'l', 0x0, ' ', 0x0, 'C', 0x0,
	'o', 0x0, 'n', 0x0, 't', 0x0, 'r', 0x0, 'o', 0x0,
	'l', 0x0
};

/* setting the device qualifier descriptor and a string descriptor */
static const u8 qualifier_desc[] = {
	0x0a,		/*  0 desc size */
	0x06,		/*  1 desc type (DEVICE_QUALIFIER) */
	0x00,		/*  2 USB release */
	0x02,		/*  3 => 2.00 */
	0xFF,		/*  4 class */
	0x00,		/*  5 subclass */
	0x00,		/*  6 protocol */
	64,		/*  7 max pack size */
	0x01,		/*  8 number of other-speed configuration */
	0x00,		/*  9 reserved */
};

static const u8 config_full[] = {
	0x09,		/*  0 desc size */
	0x07,		/*  1 desc type (other speed) */
	0x20,		/*  2 Total length of data returned */
	0x00,		/*  3 */
	0x01,		/*  4 Number of interfaces */
	0x01,		/*  5 value to use to select configuration */
	0x00,		/*  6 index of string desc */
	/*  7 same as configuration desc */
	CONF_ATTR_DEFAULT | CONF_ATTR_SELFPOWERED,
	0x19,		/*  8 same as configuration desc */

};

static const u8 config_full_total[] = {
	0x09, 0x07, 0x20, 0x00, 0x01, 0x01, 0x00, 0xC0, 0x19,
	0x09, 0x04, 0x00, 0x00, 0x02, 0xff, 0x00, 0x00, 0x00,
	0x07, 0x05, 0x83, 0x02, 0x40, 0x00, 0x00,
	0x07, 0x05, 0x04, 0x02, 0x40, 0x00, 0x00
};

static const u8 config_high[] = {
	0x09,		/*  0 desc size */
	0x07,		/*  1 desc type (other speed) */
	0x20,		/*  2 Total length of data returned */
	0x00,		/*  3 */
	0x01,		/*  4 Number of interfaces */
	0x01,		/*  5 value to use to select configuration */
	0x00,		/*  6 index of string desc */
	/*  7 same as configuration desc */
	CONF_ATTR_DEFAULT | CONF_ATTR_SELFPOWERED,
	0x19,			/*  8 same as configuration desc */

};

static const u8 config_high_total[] = {
	0x09, 0x07, 0x20, 0x00, 0x01, 0x01, 0x00, 0xC0, 0x19,
	0x09, 0x04, 0x00, 0x00, 0x02, 0xff, 0x00, 0x00, 0x00,
	0x07, 0x05, 0x81, 0x02, 0x00, 0x02, 0x00,
	0x07, 0x05, 0x02, 0x02, 0x00, 0x02, 0x00
};

/* Descriptor size */
enum DESCRIPTOR_SIZE {
	DEVICE_DESC_SIZE = sizeof(device_desc_t),
	STRING_DESC0_SIZE = sizeof(string_desc0),
	STRING_DESC1_SIZE = sizeof(string_desc1),
	STRING_DESC2_SIZE = sizeof(string_desc2),
	STRING_DESC3_SIZE = sizeof(string_desc3),
	CONFIG_DESC_SIZE = sizeof(config_desc_t),
	INTERFACE_DESC_SIZE = sizeof(intf_desc_t),
#ifdef CONFIG_S5P_USB_NON_ZLP
	AV_DESC_SIZE = sizeof(av_desc_t),
#else
	AV_DESC_SIZE = 0,
#endif
	ENDPOINT_DESC_SIZE = sizeof(ep_desc_t),
	DEVICE_QUALIFIER_SIZE = sizeof(qualifier_desc),
	OTHER_SPEED_CFG_SIZE = 9
};

/*32 <cfg desc>+<if desc>+<endp0 desc>+<endp1 desc>*/
#define CONFIG_DESC_TOTAL_SIZE	\
	(CONFIG_DESC_SIZE + INTERFACE_DESC_SIZE * 2 \
	+ AV_DESC_SIZE + ENDPOINT_DESC_SIZE * 3 \
	+ sizeof(XLLP_UDC_USB_CDC_HEADER_DESCRIPTOR_T) \
	+ sizeof(XLLP_UDC_USB_CDC_CALL_MANAGEMENT_DESCRIPTOR_T) \
	+ sizeof(XLLP_UDC_USB_CDC_ACM_DESCRIPTOR_T) \
	+ sizeof(XLLP_UDC_USB_CDC_UNION_DESCRIPTOR_T))

static unsigned int phy_base;
static unsigned int otg_base;

/* BULK XFER SIZE is 256K bytes */
#define BULK_XFER_SIZE	262144
u8 usb_ctrl[8] __attribute__((aligned(8)));
u8 ctrl_buf[128] __attribute__((aligned(8)));
u8 ctrl_out[8] __attribute__((aligned(8)));
u8 bulk_in_tmp[HS_BULK_PKT_SIZE] __attribute__((aligned(8)));

static inline void s5p_usb_init_base(void)
{
	phy_base = samsung_get_base_usb_phy();
	otg_base = samsung_get_base_usb_otg();
}

static inline int s5p_phy_read_reg(int offset)
{
	return readl(phy_base + offset);
}

static inline void s5p_phy_write_reg(int value, int offset)
{
	writel(value, phy_base + offset);
}

static inline int s5p_otg_read_reg(int offset)
{
	return readl(otg_base + offset);
}

static inline void s5p_otg_write_reg(int value, int offset)
{
	writel(value, otg_base + offset);
}

static void s5p_usb_init_phy(void)
{
	if (s5p_cpu_id == 0xc100) {
		s5p_phy_write_reg(0x0, OTG_PHYPWR);
#ifdef CONFIG_OTG_CLK_OSCC
		s5p_phy_write_reg(0x22, OTG_PHYCTRL);
#else
		s5p_phy_write_reg(0x2, OTG_PHYCTRL);
#endif
	} else if (s5p_cpu_id == 0xc110) {
		s5p_phy_write_reg(0xA0, OTG_PHYPWR);
		s5p_phy_write_reg(0x3, OTG_PHYCTRL);
	} else if (s5p_cpu_id == 0x4321) {
		s5p_phy_write_reg(0x1f80, OTG_PHYPWR);
		s5p_phy_write_reg(0x3, OTG_PHYCTRL);
	} else if (s5p_cpu_id == 0x4322) {
		s5p_phy_write_reg(0x7f80, OTG_PHYPWR);
		s5p_phy_write_reg(0x5, OTG_PHYCTRL);
	} else if (s5p_cpu_id == 0xe441) {
		s5p_phy_write_reg(0x7f80, OTG_PHYPWR);
		s5p_phy_write_reg(0x5, OTG_PHYCTRL);
	}

	s5p_phy_write_reg(0x1, OTG_RSTCON);

	/* Pumping USB Signal */
	s5p_phy_write_reg(s5p_phy_read_reg(OTG_PHYTUN1) | (0x1 << 20), OTG_PHYTUN1);

	udelay(20);
	s5p_phy_write_reg(0x0, OTG_RSTCON);
	udelay(20);
}

int s5p_usb_detect_irq(void)
{
	u32 status;
	status = s5p_otg_read_reg(OTG_GINTSTS);

#ifdef CONFIG_S5P_USB_DMA
	/*(GINTSTS_WkUpInt|GINTSTS_OEPInt|GINTSTS_IEPInt|GINTSTS_EnumDone|GINTSTS_USBRst|GINTSTS_USBSusp)*/
	return (status & 0x800c3800);
#else
	return (status & 0x800c3810);
#endif
}

void s5p_usb_clear_irq(void)
{
	s5p_otg_write_reg(0xffffffff, OTG_GINTSTS);
}

static void s5p_usb_core_soft_reset(void)
{
	u32 tmp;

	s5p_otg_write_reg(CORE_SOFT_RESET, OTG_GRSTCTL);

	do {
		tmp = s5p_otg_read_reg(OTG_GRSTCTL);
	} while (!(tmp & AHB_MASTER_IDLE));
}

#if 0
static void s5p_usb_wait_cable_insert(void)
{
	u32 tmp;
	int ucFirst = 1;

	do {
		udelay(50);

		tmp = s5p_otg_read_reg(OTG_GOTGCTL);

		if (tmp & (B_SESSION_VALID | A_SESSION_VALID)) {
			break;
		} else if (ucFirst == 1) {
			puts("Insert a OTG cable into the connector!\n");
			ucFirst = 0;
		}
	} while (1);
}
#endif

static void s5p_usb_init_core(void)
{
#ifdef CONFIG_S5P_USB_DMA
	s5p_otg_write_reg(PTXFE_HALF | NPTXFE_HALF | MODE_DMA |
			BURST_INCR4 | GBL_INT_UNMASK, OTG_GAHBCFG);
#else
	s5p_otg_write_reg(PTXFE_HALF | NPTXFE_HALF | MODE_SLAVE |
			BURST_SINGLE | GBL_INT_UNMASK, OTG_GAHBCFG);
#endif

	s5p_otg_write_reg(
		 0x0 << 15	/* PHY Low Power Clock sel */
	       | 0x1 << 14	/* Non-Periodic TxFIFO Rewind Enable */
	       | 0x5 << 10	/* Turnaround time */
	       | 0x0 << 9	/* 0:HNP disable, 1:HNP enable */
	       | 0x0 << 8	/* 0:SRP disable, 1:SRP enable */
	       | 0x0 << 7	/* ULPI DDR sel */
	       | 0x0 << 6	/* 0: high speed utmi+, 1: full speed serial */
	       | 0x0 << 4	/* 0: utmi+, 1:ulpi */
	       | 0x1 << 3	/* phy i/f  0:8bit, 1:16bit */
	       | 0x7 << 0,	/* HS/FS Timeout* */
	       OTG_GUSBCFG);
}

static void s5p_usb_check_current_mode(u8 *pucMode)
{
	u32 tmp;

	tmp = s5p_otg_read_reg(OTG_GINTSTS);
	*pucMode = tmp & 0x1;
}
#if 0
int s5p_usb_status_check(void)
{
	u32 status;
	status = s5p_otg_read_reg(OTG_GINTSTS);

	/* OTG interrupt check. */
	if (status & (0x1 << 2)) {
		/* Session end? */
		if (s5p_otg_read_reg(OTG_GOTGINT) & (0x1 << 2)) {
			s5p_otg_write_reg((0x1 << 2), OTG_GOTGINT);
			puts("Error: Cable disconnected!\n");

			while(1);
		}
	}

	return 0;
}
#endif
static void s5p_usb_soft_disconnect(int set)
{
	u32 tmp;

	tmp = s5p_otg_read_reg(OTG_DCTL);
	if (set)
		tmp |= SOFT_DISCONNECT;
	else
		tmp &= ~SOFT_DISCONNECT;
	s5p_otg_write_reg(tmp, OTG_DCTL);
}

static void s5p_usb_init_device(void)
{
	s5p_otg_write_reg(1 << 18 | otg.speed << 0, OTG_DCFG);

#ifdef CONFIG_S5P_USB_DMA
	s5p_otg_write_reg(INT_RESUME | INT_OUT_EP | INT_IN_EP |
			INT_ENUMDONE | INT_RESET | INT_SUSPEND ,
			OTG_GINTMSK);
#else
	s5p_otg_write_reg(INT_RESUME | INT_OUT_EP | INT_IN_EP |
			INT_ENUMDONE | INT_RESET | INT_SUSPEND |
			INT_RX_FIFO_NOT_EMPTY, OTG_GINTMSK);
#endif
}

int s5p_usbctl_init(void)
{
	u8 ucMode;
	u32 reg;

	s5p_usb_init_base();

	reg = readl(s5p_get_usb_power_reg());

	if (s5p_cpu_id == 0xc100)
		reg |= (1 << 16);	/* unmask usb signal */
	else
		reg |= (1 << 0);	/* USB PHY0 enable */

	writel(reg, s5p_get_usb_power_reg());

	otg.speed = speed;
	otg.set_config = 0;
	otg.ep0_state = EP0_STATE_INIT;
	otg.ep0_substate = 0;

	s5p_usb_init_phy();
	s5p_usb_core_soft_reset();
	s5p_usb_init_core();
	s5p_usb_check_current_mode(&ucMode);

	if (ucMode == INT_DEV_MODE) {
		s5p_usb_soft_disconnect(1);
		udelay(10);
		s5p_usb_soft_disconnect(0);
		s5p_usb_init_device();
		return 0;
	} else {
		puts("Error : Current Mode is Host\n");
		return -1;
	}
}

static void s5p_usb_set_inep_xfersize(EP_TYPE type, u32 pktcnt, u32 xfersize)
{
	if (type == EP_TYPE_CONTROL) {
		s5p_otg_write_reg((pktcnt << 19) | (xfersize << 0),
				OTG_DIEPTSIZ0);
	} else if (type == EP_TYPE_BULK) {
		s5p_otg_write_reg((1 << 29) | (pktcnt << 19) |
				(xfersize << 0), OTG_DIEPTSIZ_IN);
	}
}

static void s5p_usb_set_outep_xfersize(EP_TYPE type, u32 pktcnt, u32 xfersize)
{
	if (type == EP_TYPE_CONTROL) {
		s5p_otg_write_reg((1 << 29) | (pktcnt << 19) |
				(xfersize << 0), OTG_DOEPTSIZ0);
	} else if (type == EP_TYPE_BULK) {
		s5p_otg_write_reg((pktcnt << 19) | (xfersize << 0),
				OTG_DOEPTSIZ_OUT);
	}
}

#ifndef CONFIG_S5P_USB_DMA
/* works on both aligned and unaligned buffers */
static void s5p_usb_write_ep0_fifo(u8 *buf, int num)
{
	int i, j;
	u32 Wr_Data=0;
	int int_status;
	int parts = 0;

	/* calculate number of chunks to be sent */
	parts = (num % 64) ? num / 64 + 1 : num / 64;
	for (j = 0; j < parts; j++){
		s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, parts - j, num > 64 ? 64 : num);
		for (i = 0; i < num && i < 64; i += 4) {
			Wr_Data = ((*(buf+3))<<24)|((*(buf+2))<<16)|((*(buf+1))<<8)|*buf;
			s5p_otg_write_reg(Wr_Data, OTG_EP0_FIFO);
			buf += 4;
		}
		num -= 64;

		/* Some data remains */
		if (num > 0) {
			while (1) {
				int_status = s5p_otg_read_reg(OTG_GINTSTS);
				if (int_status & (1 << 18)) {
					udelay(50);
					s5p_otg_write_reg(1 << 18, OTG_GINTSTS);
					/* set partial pkt size */
					s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, num - 64);
					break;
				}
			}
		}
	}
}
#endif

/* optimized fifo access routines, warning: only aligned buffers are supported */
static inline void s5p_usb_write_in_fifo(u8 *buf, int num)
{
	u32 fifo = otg_base + OTG_IN_FIFO;
	u32 *p = (u32 *)buf;
	int i;

	for (i = 0; i < num; i += 4)
		writel(*p++, fifo);
}

static inline void s5p_usb_read_out_fifo(u8 *buf, int num)
{
	u32 fifo = otg_base + OTG_OUT_FIFO;
	u32 *p = (u32 *)buf;
	int i;

	for (i = 0; i < num; i += 4)
		*p++ = readl(fifo);
}

static void s5p_usb_get_desc(void)
{
	otg.req_length = (u32)((otg.dev_req.wLength_H << 8) |
			otg.dev_req.wLength_L);

	switch (otg.dev_req.wValue_H) {
	case DEVICE_DESCRIPTOR:
		otg.ep0_state = EP0_STATE_GD_DEV_0;
		break;

	case CONFIGURATION_DESCRIPTOR:
		if (otg.req_length > CONFIG_DESC_SIZE)
			otg.ep0_state = EP0_STATE_GD_CFG_0;
		else
			otg.ep0_state = EP0_STATE_GD_CFG_ONLY_0;
		break;

	case STRING_DESCRIPTOR:
		switch (otg.dev_req.wValue_L) {
		case 0:
			otg.ep0_state = EP0_STATE_GD_STR_I0;
			break;
		case 1:
			otg.ep0_state = EP0_STATE_GD_STR_I1;
			break;
		case 2:
			otg.ep0_state = EP0_STATE_GD_STR_I2;
			break;
		default:
			break;
		}
		break;

	case ENDPOINT_DESCRIPTOR:
		switch (otg.dev_req.wValue_L & 0xf) {
		case 0:
			otg.ep0_state = EP0_STATE_GD_EP0_ONLY_0;
			break;
		case 1:
			otg.ep0_state = EP0_STATE_GD_EP1_ONLY_0;
			break;
		default:
			break;
		}
		break;

	case DEVICE_QUALIFIER:
		otg.ep0_state = EP0_STATE_GD_DEV_QUALIFIER;
		break;

	case OTHER_SPEED_CONFIGURATION:
		otg.ep0_state = EP0_STATE_GD_OTHER_SPEED;
		break;
	}
}

static void s5p_usb_clear_feature(void)
{
	switch (otg.dev_req.bmRequestType) {
	case DEVICE_RECIPIENT:
		if (otg.dev_req.wValue_L == 1)
			remode_wakeup = 0;
		break;

	case ENDPOINT_RECIPIENT:
		if (otg.dev_req.wValue_L == 0) {
			if ((otg.dev_req.wIndex_L & 0x7f) == CONTROL_EP)
				get_status.ep_ctrl = 0;

			/* IN Endpoint */
			if ((otg.dev_req.wIndex_L & 0x7f) == BULK_IN_EP)
				get_status.ep_in = 0;

			/* OUT Endpoint */
			if ((otg.dev_req.wIndex_L & 0x7f) == BULK_OUT_EP)
				get_status.ep_out = 0;
		}
		break;

	default:
		break;
	}
	otg.ep0_state = EP0_STATE_INIT;
}

static void s5p_usb_set_feature(void)
{
	switch (otg.dev_req.bmRequestType) {
	case DEVICE_RECIPIENT:
		if (otg.dev_req.wValue_L == 1)
			remode_wakeup = 1;
		break;

	case ENDPOINT_RECIPIENT:
		if (otg.dev_req.wValue_L == 0) {
			if ((otg.dev_req.wIndex_L & 0x7f) == CONTROL_EP)
				get_status.ep_ctrl = 1;

			if ((otg.dev_req.wIndex_L & 0x7f) == BULK_IN_EP)
				get_status.ep_in = 1;

			if ((otg.dev_req.wIndex_L & 0x7f) == BULK_OUT_EP)
				get_status.ep_out = 1;
		}
		break;

	default:
		break;
	}

	otg.ep0_state = EP0_STATE_INIT;
}

static void s5p_usb_get_status(void)
{
	switch (otg.dev_req.bmRequestType) {
	case (0x80):	/*device */
		get_status.Device = ((u8) remode_wakeup << 1) | 0x1;
		otg.ep0_state = EP0_STATE_GET_STATUS0;
		break;

	case (0x81):	/*interface */
		get_status.Interface = 0;
		otg.ep0_state = EP0_STATE_GET_STATUS1;
		break;

	case (0x82):	/*endpoint */
		if ((otg.dev_req.wIndex_L & 0x7f) == CONTROL_EP)
			otg.ep0_state = EP0_STATE_GET_STATUS2;

		if ((otg.dev_req.wIndex_L & 0x7f) == BULK_IN_EP)
			otg.ep0_state = EP0_STATE_GET_STATUS3;

		if ((otg.dev_req.wIndex_L & 0x7f) == BULK_OUT_EP)
			otg.ep0_state = EP0_STATE_GET_STATUS4;
		break;

	default:
		break;
	}
}

static void s5p_usb_transfer_ep0(void);
static void s5p_usb_ep0_int_hndlr(void)
{
	u16 addr;

	if (otg.ep0_state == EP0_STATE_INIT) {
#ifdef CONFIG_S5P_USB_DMA
		otg.dev_req.bmRequestType = usb_ctrl[0];
		otg.dev_req.bRequest = usb_ctrl[1];
		otg.dev_req.wValue_L = usb_ctrl[2];
		otg.dev_req.wValue_H = usb_ctrl[3];
		otg.dev_req.wIndex_L = usb_ctrl[4];
		otg.dev_req.wIndex_H = usb_ctrl[5];
		otg.dev_req.wLength_L = usb_ctrl[6];
		otg.dev_req.wLength_H = usb_ctrl[7];
#else
		u16 i;
		u32 buf[2] = {0x0000, };

		for (i = 0; i < 2; i++)
			buf[i] = s5p_otg_read_reg(OTG_EP0_FIFO);

		otg.dev_req.bmRequestType = buf[0];
		otg.dev_req.bRequest = buf[0] >> 8;
		otg.dev_req.wValue_L = buf[0] >> 16;
		otg.dev_req.wValue_H = buf[0] >> 24;
		otg.dev_req.wIndex_L = buf[1];
		otg.dev_req.wIndex_H = buf[1] >> 8;
		otg.dev_req.wLength_L = buf[1] >> 16;
		otg.dev_req.wLength_H = buf[1] >> 24;
#endif

		switch (otg.dev_req.bRequest) {
		case STANDARD_SET_ADDRESS:
			/* Set Address Update bit */
			addr = (otg.dev_req.wValue_L);
			s5p_otg_write_reg(1 << 18 | addr << 4 | otg.speed << 0,
			       OTG_DCFG);
			otg.ep0_state = EP0_STATE_INIT;
			break;

		case STANDARD_SET_DESCRIPTOR:
			break;

		case STANDARD_SET_CONFIGURATION:
			/* Configuration value in configuration descriptor */
			config_value = otg.dev_req.wValue_L;
			otg.set_config = 1;
			otg.ep0_state = EP0_STATE_INIT;

#ifdef CONFIG_S5P_USB_DMA
			/* Set the Dedicated FIFO 1 for Bulk IN EP(1<<22) */
			s5p_otg_write_reg(1<<28|1<<22|2<<18|1<<15|otg.bulkin_max_pktsize<<0,OTG_DIEPCTL_IN);
			s5p_otg_write_reg(1<<28|2<<18|1<<15|otg.bulkout_max_pktsize<<0,OTG_DOEPCTL_OUT);
#endif
			s5p_usb_connected = 1;
			break;

		case STANDARD_GET_CONFIGURATION:
#ifdef CONFIG_S5P_USB_DMA
			s5p_usb_setdma((void *)&config_value, 1, BULK_IN_TYPE);
#else
			s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 1);

			/*ep0 enable, clear nak, next ep0, 8byte */
			s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
			s5p_otg_write_reg(config_value, OTG_EP0_FIFO);
#endif
			otg.ep0_state = EP0_STATE_INIT;
			break;

		case STANDARD_GET_DESCRIPTOR:
			s5p_usb_get_desc();
			break;

		case STANDARD_CLEAR_FEATURE:
			s5p_usb_clear_feature();
			break;

		case STANDARD_SET_FEATURE:
			s5p_usb_set_feature();
			break;

		case STANDARD_GET_STATUS:
			s5p_usb_get_status();
			break;

		case STANDARD_GET_INTERFACE:
			otg.ep0_state = EP0_STATE_INTERFACE_GET;
			break;

		case STANDARD_SET_INTERFACE:
			get_intf.AlternateSetting = otg.dev_req.wValue_L;
			otg.ep0_state = EP0_STATE_INIT;
			break;

		case STANDARD_SYNCH_FRAME:
			otg.ep0_state = EP0_STATE_INIT;
			break;

		case STANDARD_RESERVED_1:
#ifdef CONFIG_S5P_USB_DMA
			/* to handle CDC Class request OUT (0x02) case*/
			if ( otg.dev_req.bmRequestType == 0x21 ) {
				s5p_usb_setdma((void *)ctrl_out, 2, CTRL_OUT_TYPE);
				otg.ep0_state = EP0_STATE_INIT;
			}
#endif
			break;

		case STANDARD_RESERVED_3:
#ifdef CONFIG_S5P_USB_DMA
			/* to handle CDC Class request OUT (0x20) case*/
			if ( otg.dev_req.bmRequestType == 0x21 ) {
				s5p_usb_setdma((void *)ctrl_out, 7, CTRL_OUT_TYPE);
				otg.ep0_state = EP0_STATE_INIT;
			}
#endif
			break;

		default:
			break;
		}
	}

#ifdef CONFIG_S5P_USB_DMA
	s5p_usb_transfer_ep0();
#else
	s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, otg.ctrl_max_pktsize);

	/*clear nak, next ep0, 64byte */
	s5p_otg_write_reg(((1 << 26) | (CONTROL_EP << 11) | (0 << 0)),
			OTG_DIEPCTL0);
#endif
}

static void s5p_usb_set_otherspeed_conf_desc(u32 length)
{
	/* Standard device descriptor */
#ifdef CONFIG_S5P_USB_DMA
	if (length == 9) {
		s5p_usb_setdma(((void *)&config_full)+0, 9, CTRL_IN_TYPE);
	} else if (length == 32) {
		s5p_usb_setdma(((void *)&config_full_total)+0, 32, CTRL_IN_TYPE);
	}
#else
	if (length == 9) {
		s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 9);
		s5p_otg_write_reg(EPEN_CNAK_EP0_64, OTG_DIEPCTL0);
		s5p_usb_write_ep0_fifo((u8 *) &config_full, 9);
	} else if (length == 32) {
		s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 32);
		s5p_otg_write_reg(EPEN_CNAK_EP0_64, OTG_DIEPCTL0);
		s5p_usb_write_ep0_fifo((u8 *) &config_full_total, 32);
	}
#endif
	otg.ep0_state = EP0_STATE_INIT;
}

static void s5p_usb_transfer_ep0(void)
{
	u32 size;
	u8 *desc;

	switch (otg.ep0_state) {
	case EP0_STATE_INIT:
#ifdef CONFIG_S5P_USB_DMA
		s5p_usb_setdma((void *)&zlp_buf, 0, CTRL_IN_TYPE);
#else
		s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 0);

		/*ep0 enable, clear nak, next ep0, 8byte */
		s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
#endif
		break;

	case EP0_STATE_GD_DEV_0:
#ifdef CONFIG_S5P_USB_DMA
		if (otg.req_length < DEVICE_DESC_SIZE) {
			s5p_usb_setdma(((void *)&otg.desc.dev)+0, otg.req_length, CTRL_IN_TYPE);
		} else {
			s5p_usb_setdma(((void *)&otg.desc.dev)+0, DEVICE_DESC_SIZE, CTRL_IN_TYPE);
		}
#else
		/*ep0 enable, clear nak, next ep0, max 64byte */
		s5p_otg_write_reg(EPEN_CNAK_EP0_64, OTG_DIEPCTL0);
		if (otg.req_length < DEVICE_DESC_SIZE) {
			s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1,
					otg.req_length);
			s5p_usb_write_ep0_fifo(((u8 *) &otg.desc.dev),
					otg.req_length);
		} else {
			s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1,
					DEVICE_DESC_SIZE);
			s5p_usb_write_ep0_fifo(((u8 *) &otg.desc.dev),
					DEVICE_DESC_SIZE);
		}
#endif
		otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GD_DEV_1:
#ifdef CONFIG_S5P_USB_DMA
		if (otg.req_length < (2 * FS_CTRL_PKT_SIZE)) {
			s5p_usb_setdma(((void *)&otg.desc.dev)+FS_CTRL_PKT_SIZE, otg.req_length - FS_CTRL_PKT_SIZE, CTRL_IN_TYPE);
			otg.ep0_state = EP0_STATE_INIT;
		} else {
			s5p_usb_setdma(((void *)&otg.desc.dev)+FS_CTRL_PKT_SIZE, FS_CTRL_PKT_SIZE, CTRL_IN_TYPE);
			otg.ep0_state = EP0_STATE_GD_DEV_2;
		}
#else
		s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
		if (otg.req_length < (2 * FS_CTRL_PKT_SIZE)) {
			s5p_usb_write_ep0_fifo(((u8 *) &otg.desc.dev) +
					FS_CTRL_PKT_SIZE,
					otg.req_length - FS_CTRL_PKT_SIZE);
			otg.ep0_state = EP0_STATE_INIT;
		} else {
			s5p_usb_write_ep0_fifo(((u8 *) &otg.desc.dev) +
					FS_CTRL_PKT_SIZE, FS_CTRL_PKT_SIZE);
			otg.ep0_state = EP0_STATE_GD_DEV_2;
		}
#endif
		break;

	case EP0_STATE_GD_DEV_2:
#ifdef CONFIG_S5P_USB_DMA
		if (otg.req_length < DEVICE_DESC_SIZE) {
			s5p_usb_setdma(((void *)&otg.desc.dev)+(2 * FS_CTRL_PKT_SIZE),
				otg.req_length - 2 * FS_CTRL_PKT_SIZE, CTRL_IN_TYPE);
		} else {
			s5p_usb_setdma(((void *)&otg.desc.dev)+(2 * FS_CTRL_PKT_SIZE),
				DEVICE_DESC_SIZE - 2 * FS_CTRL_PKT_SIZE, CTRL_IN_TYPE);
		}
#else
		s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
		if (otg.req_length < DEVICE_DESC_SIZE) {
			s5p_usb_write_ep0_fifo(((u8 *) &otg.desc.dev) +
					(2 * FS_CTRL_PKT_SIZE),
					otg.req_length - 2 * FS_CTRL_PKT_SIZE);
		} else {
			s5p_usb_write_ep0_fifo(((u8 *) &otg.desc.dev) +
					(2 * FS_CTRL_PKT_SIZE),
					DEVICE_DESC_SIZE - 2 * FS_CTRL_PKT_SIZE);
		}
#endif
		otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GD_CFG_0:
		size = CONFIG_DESC_TOTAL_SIZE;

#ifdef CONFIG_S5P_USB_DMA
		if (otg.req_length < size) {
			s5p_usb_setdma(((void *)&otg.desc.config)+0, otg.req_length, CTRL_IN_TYPE);
		} else {
			s5p_usb_setdma(((void *)&otg.desc.config)+0, size, CTRL_IN_TYPE);
		}
#else
		s5p_otg_write_reg(EPEN_CNAK_EP0_64, OTG_DIEPCTL0);
		if (otg.req_length < size) {
			s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1,
					otg.req_length);
			s5p_usb_write_ep0_fifo(((u8 *) &otg.desc.config),
					otg.req_length);
		} else {
			s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, size);
			s5p_usb_write_ep0_fifo(((u8 *)&otg.desc.config), size);
		}
#endif
		otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GD_CFG_1:
#ifdef CONFIG_S5P_USB_DMA
		if (otg.req_length < (2 * FS_CTRL_PKT_SIZE)) {
			s5p_usb_setdma(((void *)&otg.desc.config)+FS_CTRL_PKT_SIZE, (otg.req_length - FS_CTRL_PKT_SIZE), CTRL_IN_TYPE);
			otg.ep0_state = EP0_STATE_INIT;
		} else {
			s5p_usb_setdma(((void *)&otg.desc.config)+FS_CTRL_PKT_SIZE, FS_CTRL_PKT_SIZE, CTRL_IN_TYPE);
			otg.ep0_state = EP0_STATE_GD_CFG_2;
		}
#else
		s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
		if (otg.req_length < (2 * FS_CTRL_PKT_SIZE)) {
			s5p_usb_write_ep0_fifo(((u8 *) &otg.desc.config) +
					FS_CTRL_PKT_SIZE,
					(otg.req_length - FS_CTRL_PKT_SIZE));
			otg.ep0_state = EP0_STATE_INIT;
		} else {
			s5p_usb_write_ep0_fifo(((u8 *) &otg.desc.config) +
					FS_CTRL_PKT_SIZE, FS_CTRL_PKT_SIZE);
			otg.ep0_state = EP0_STATE_GD_CFG_2;
		}
#endif
		break;

	case EP0_STATE_GD_CFG_2:
#ifdef CONFIG_S5P_USB_DMA
		if (otg.req_length < (3 * FS_CTRL_PKT_SIZE)) {
			s5p_usb_setdma(((void *)&otg.desc.config)+(2 * FS_CTRL_PKT_SIZE),
				(otg.req_length - 2 * FS_CTRL_PKT_SIZE), CTRL_IN_TYPE);
			otg.ep0_state = EP0_STATE_INIT;
		} else {
			s5p_usb_setdma(((void *)&otg.desc.config)+(2 * FS_CTRL_PKT_SIZE),
				FS_CTRL_PKT_SIZE, CTRL_IN_TYPE);
			otg.ep0_state = EP0_STATE_GD_CFG_3;
		}
#else
		s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
		if (otg.req_length < (3 * FS_CTRL_PKT_SIZE)) {
			s5p_usb_write_ep0_fifo(((u8 *) &otg.desc.config) +
					(2 * FS_CTRL_PKT_SIZE),
					otg.req_length - 2 * FS_CTRL_PKT_SIZE);
			otg.ep0_state = EP0_STATE_INIT;
		} else {
			s5p_usb_write_ep0_fifo(((u8 *) &otg.desc.config) +
					(2 * FS_CTRL_PKT_SIZE), FS_CTRL_PKT_SIZE);
			otg.ep0_state = EP0_STATE_GD_CFG_3;
		}
#endif
		break;

	case EP0_STATE_GD_CFG_3:
#ifdef CONFIG_S5P_USB_DMA
		if (otg.req_length < (4 * FS_CTRL_PKT_SIZE)) {
			s5p_usb_setdma(((void *)&otg.desc.config)+(3 * FS_CTRL_PKT_SIZE),
				(otg.req_length - 3 * FS_CTRL_PKT_SIZE), CTRL_IN_TYPE);
			otg.ep0_state = EP0_STATE_INIT;
		} else {
			s5p_usb_setdma(((void *)&otg.desc.config)+(3 * FS_CTRL_PKT_SIZE),
				FS_CTRL_PKT_SIZE, CTRL_IN_TYPE);
			otg.ep0_state = EP0_STATE_GD_CFG_4;
		}
#else
		s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
		if (otg.req_length < (4 * FS_CTRL_PKT_SIZE)) {
			s5p_usb_write_ep0_fifo(((u8 *) &otg.desc.config) +
					(3 * FS_CTRL_PKT_SIZE),
					otg.req_length - 3 * FS_CTRL_PKT_SIZE);
			otg.ep0_state = EP0_STATE_INIT;
		} else {
			s5p_usb_write_ep0_fifo(((u8 *) &otg.desc.config) +
					(3 * FS_CTRL_PKT_SIZE), FS_CTRL_PKT_SIZE);
			otg.ep0_state = EP0_STATE_GD_CFG_4;
		}
#endif
		break;

	case EP0_STATE_GD_CFG_4:
		otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GD_DEV_QUALIFIER:
#ifdef CONFIG_S5P_USB_DMA
		if (otg.req_length < 10) {
			s5p_usb_setdma(((void *)qualifier_desc)+0, otg.req_length, CTRL_IN_TYPE);
		} else {
			s5p_usb_setdma(((void *)qualifier_desc)+0, 10, CTRL_IN_TYPE);
		}
#else
		s5p_otg_write_reg(EPEN_CNAK_EP0_64, OTG_DIEPCTL0);
		if (otg.req_length < 10) {
			s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1,
					otg.req_length);
			s5p_usb_write_ep0_fifo((u8 *)qualifier_desc,
					otg.req_length);
		} else {
			s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 10);
			s5p_usb_write_ep0_fifo((u8 *)qualifier_desc, 10);
		}
#endif
		otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GD_OTHER_SPEED:
		s5p_usb_set_otherspeed_conf_desc(otg.req_length);
		break;

	case EP0_STATE_GD_OTHER_SPEED_HIGH_1:
#ifdef CONFIG_S5P_USB_DMA
		if (otg.req_length == 9) {
			s5p_usb_setdma(((void *)config_high)+8, 1, CTRL_IN_TYPE);
			otg.ep0_state = EP0_STATE_INIT;
		} else {
			s5p_usb_setdma(((void *)config_high)+8, 8, CTRL_IN_TYPE);
			otg.ep0_state = EP0_STATE_GD_OTHER_SPEED_HIGH_2;
		}
#else
		if (otg.req_length == 9) {
			s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 1);
			s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
			s5p_usb_write_ep0_fifo(((u8 *) &config_high) + 8, 1);
			otg.ep0_state = EP0_STATE_INIT;
		} else {
			s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 8);
			s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
			s5p_usb_write_ep0_fifo(((u8 *) &config_high) + 8, 8);
			otg.ep0_state = EP0_STATE_GD_OTHER_SPEED_HIGH_2;
		}
#endif
		break;

	case EP0_STATE_GD_OTHER_SPEED_HIGH_2:
#ifdef CONFIG_S5P_USB_DMA
		s5p_usb_setdma(((void *)config_high)+16, 8, CTRL_IN_TYPE);
#else
		s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 8);
		s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
		s5p_usb_write_ep0_fifo(((u8 *) &config_high) + 16, 8);
#endif
		otg.ep0_state = EP0_STATE_GD_OTHER_SPEED_HIGH_3;
		break;

	case EP0_STATE_GD_OTHER_SPEED_HIGH_3:
#ifdef CONFIG_S5P_USB_DMA
		s5p_usb_setdma(((void *)config_high)+24, 8, CTRL_IN_TYPE);
#else
		s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 8);
		s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
		s5p_usb_write_ep0_fifo(((u8 *) &config_high) + 24, 8);
#endif
		otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GD_CFG_ONLY_0:
#ifdef CONFIG_S5P_USB_DMA
		if (otg.req_length < CONFIG_DESC_SIZE) {
			s5p_usb_setdma(((void *)&otg.desc.config)+0, otg.req_length, CTRL_IN_TYPE);
		} else {
			s5p_usb_setdma(((void *)&otg.desc.config)+0, CONFIG_DESC_SIZE, CTRL_IN_TYPE);
		}
#else
		if (otg.req_length < CONFIG_DESC_SIZE) {
			s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1,
					otg.req_length);
			s5p_otg_write_reg(EPEN_CNAK_EP0_64, OTG_DIEPCTL0);
			s5p_usb_write_ep0_fifo(((u8 *) &otg.desc.config),
					otg.req_length);
		} else {
			s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1,
					CONFIG_DESC_SIZE);
			s5p_otg_write_reg(EPEN_CNAK_EP0_64, OTG_DIEPCTL0);
			s5p_usb_write_ep0_fifo(((u8 *) &otg.desc.config),
					CONFIG_DESC_SIZE);
		}
#endif
		otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GD_CFG_ONLY_1:
#ifdef CONFIG_S5P_USB_DMA
		s5p_usb_setdma(((void *)&otg.desc.config)+FS_CTRL_PKT_SIZE,
				(CONFIG_DESC_SIZE - FS_CTRL_PKT_SIZE), CTRL_IN_TYPE);
#else
		s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
		s5p_usb_write_ep0_fifo(((u8 *) &otg.desc.config) +
				FS_CTRL_PKT_SIZE,
				CONFIG_DESC_SIZE - FS_CTRL_PKT_SIZE);
#endif
		otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GD_IF_ONLY_0:
#ifdef CONFIG_S5P_USB_DMA
		if (otg.req_length < INTERFACE_DESC_SIZE) {
			s5p_usb_setdma(((void *)&otg.desc.intf)+0,	otg.req_length, CTRL_IN_TYPE);
		} else {
			s5p_usb_setdma(((void *)&otg.desc.intf)+0,	INTERFACE_DESC_SIZE, CTRL_IN_TYPE);
		}
#else
		if (otg.req_length < INTERFACE_DESC_SIZE) {
			s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1,
					otg.req_length);
			s5p_otg_write_reg(EPEN_CNAK_EP0_64, OTG_DIEPCTL0);
			s5p_usb_write_ep0_fifo(((u8 *) &otg.desc.intf),
					otg.req_length);
		} else {
			s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1,
					INTERFACE_DESC_SIZE);
			s5p_otg_write_reg(EPEN_CNAK_EP0_64, OTG_DIEPCTL0);
			s5p_usb_write_ep0_fifo(((u8 *) &otg.desc.intf),
					INTERFACE_DESC_SIZE);
		}
#endif
		otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GD_IF_ONLY_1:
#ifdef CONFIG_S5P_USB_DMA
		s5p_usb_setdma(((void *)&otg.desc.intf)+FS_CTRL_PKT_SIZE,
				(INTERFACE_DESC_SIZE - FS_CTRL_PKT_SIZE), CTRL_IN_TYPE);
#else
		s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
		s5p_usb_write_ep0_fifo(((u8 *) &otg.desc.intf) +
				FS_CTRL_PKT_SIZE,
				INTERFACE_DESC_SIZE - FS_CTRL_PKT_SIZE);
#endif
		otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GD_EP0_ONLY_0:
#ifdef CONFIG_S5P_USB_DMA
		s5p_usb_setdma(((void *)&otg.desc.ep1)+0,	ENDPOINT_DESC_SIZE, CTRL_IN_TYPE);
#else
		s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
		s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, ENDPOINT_DESC_SIZE);
		s5p_usb_write_ep0_fifo(((u8 *) &otg.desc.ep1),
				ENDPOINT_DESC_SIZE);
#endif
		otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GD_EP1_ONLY_0:
#ifdef CONFIG_S5P_USB_DMA
		s5p_usb_setdma(((void *)&otg.desc.ep2)+0, ENDPOINT_DESC_SIZE, CTRL_IN_TYPE);
#else
		s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, ENDPOINT_DESC_SIZE);
		s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
		s5p_usb_write_ep0_fifo(((u8 *) &otg.desc.ep2),
				ENDPOINT_DESC_SIZE);
#endif
		otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GD_STR_I0:
#ifdef CONFIG_S5P_USB_DMA
		if (otg.req_length < STRING_DESC0_SIZE)
			s5p_usb_setdma(((void *)string_desc0)+0, otg.req_length, CTRL_IN_TYPE);
		else
			s5p_usb_setdma(((void *)string_desc0)+0, STRING_DESC0_SIZE, CTRL_IN_TYPE);
#else
		if (otg.req_length < STRING_DESC0_SIZE) {
			s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, otg.req_length);
			s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
			s5p_usb_write_ep0_fifo((u8 *)string_desc0, otg.req_length);
		} else {
			s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, STRING_DESC0_SIZE);
			s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
			s5p_usb_write_ep0_fifo((u8 *)string_desc0, STRING_DESC0_SIZE);
		}
#endif
		otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GD_STR_I1:
#ifdef CONFIG_S5P_USB_DMA
		if (otg.req_length < sizeof(string_desc1)) {
			s5p_usb_setdma((void *)(string_desc1 + 0), otg.req_length, CTRL_IN_TYPE);
			otg.ep0_state = EP0_STATE_INIT;
		} else {
			if ((otg.ep0_substate * otg.ctrl_max_pktsize +
				otg.ctrl_max_pktsize) <	sizeof(string_desc1)) {
				s5p_usb_setdma((void *)(string_desc1 +
						(otg.ep0_substate * otg.ctrl_max_pktsize)),
						otg.ctrl_max_pktsize, CTRL_IN_TYPE);
				otg.ep0_state = EP0_STATE_GD_STR_I1;
				otg.ep0_substate++;
			} else {
				s5p_usb_setdma(string_desc1 +
						(otg.ep0_substate * otg.ctrl_max_pktsize),
						sizeof(string_desc1) - (otg.ep0_substate *
							otg.ctrl_max_pktsize), CTRL_IN_TYPE);
				otg.ep0_state = EP0_STATE_INIT;
				otg.ep0_substate = 0;
			}
		}
#else
		if (otg.req_length < sizeof(string_desc1))
			s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, otg.req_length);
		else
			s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, sizeof(string_desc1));

		s5p_otg_write_reg(EPEN_CNAK_EP0_64, OTG_DIEPCTL0);

		if (otg.req_length < sizeof(string_desc1)) {
			s5p_usb_write_ep0_fifo(string_desc1 + 0, otg.req_length);
			otg.ep0_state = EP0_STATE_INIT;
		} else {
			if ((otg.ep0_substate * otg.ctrl_max_pktsize +
				otg.ctrl_max_pktsize) <	sizeof(string_desc1)) {

				s5p_usb_write_ep0_fifo(string_desc1 +
						(otg.ep0_substate * otg.ctrl_max_pktsize),
						otg.ctrl_max_pktsize);
				otg.ep0_state = EP0_STATE_GD_STR_I1;
				otg.ep0_substate++;
			} else {
				s5p_usb_write_ep0_fifo(string_desc1 +
						(otg.ep0_substate * otg.ctrl_max_pktsize),
						sizeof(string_desc1) - (otg.ep0_substate *
							otg.ctrl_max_pktsize));
				otg.ep0_state = EP0_STATE_INIT;
				otg.ep0_substate = 0;
			}
		}
#endif
		break;

	case EP0_STATE_GD_STR_I2:
#ifdef CONFIG_S5P_USB_DMA
		if (otg.req_length < sizeof(string_desc2)) {
			s5p_usb_setdma(string_desc2 + 0, otg.req_length, CTRL_IN_TYPE);
			otg.ep0_state = EP0_STATE_INIT;
		} else {
			if ((otg.ep0_substate * otg.ctrl_max_pktsize +
				otg.ctrl_max_pktsize) <	sizeof(string_desc2)) {
				s5p_usb_setdma(string_desc2 +
						(otg.ep0_substate * otg.ctrl_max_pktsize),
						otg.ctrl_max_pktsize, CTRL_IN_TYPE);
				otg.ep0_state = EP0_STATE_GD_STR_I2;
				otg.ep0_substate++;
			} else {
				s5p_usb_setdma(string_desc2 +
						(otg.ep0_substate * otg.ctrl_max_pktsize),
						sizeof(string_desc2) - (otg.ep0_substate *
							otg.ctrl_max_pktsize), CTRL_IN_TYPE);
				otg.ep0_state = EP0_STATE_INIT;
				otg.ep0_substate = 0;
			}
		}
#else
		if (otg.req_length < sizeof(string_desc2))
			s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, otg.req_length);
		else
			s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, sizeof(string_desc2));

		s5p_otg_write_reg(EPEN_CNAK_EP0_64, OTG_DIEPCTL0);

		if (otg.req_length < sizeof(string_desc2)) {
			s5p_usb_write_ep0_fifo(string_desc2 + 0, otg.req_length);
			otg.ep0_state = EP0_STATE_INIT;
		} else {
			if ((otg.ep0_substate * otg.ctrl_max_pktsize +
				otg.ctrl_max_pktsize) < sizeof(string_desc2)) {
				s5p_usb_write_ep0_fifo(string_desc2 +
						(otg.ep0_substate * otg.ctrl_max_pktsize),
						otg.ctrl_max_pktsize);
				otg.ep0_state = EP0_STATE_GD_STR_I2;
				otg.ep0_substate++;
			} else {
				s5p_usb_write_ep0_fifo(string_desc2 +
						(otg.ep0_substate * otg.ctrl_max_pktsize),
						sizeof(string_desc2) - (otg.ep0_substate *
							otg.ctrl_max_pktsize));
				otg.ep0_state = EP0_STATE_INIT;
				otg.ep0_substate = 0;
			}
		}
#endif
		break;

	case EP0_STATE_INTERFACE_GET:
#ifdef CONFIG_S5P_USB_DMA
		s5p_usb_setdma(((void *)&get_intf) + 0, 1, CTRL_IN_TYPE);
#else
		s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 1);
		s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
		s5p_usb_write_ep0_fifo((u8 *) &get_intf, 1);
#endif
		otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GET_STATUS0:
#ifdef CONFIG_S5P_USB_DMA
		s5p_usb_setdma(((void *)&get_status) + 0, 1, CTRL_IN_TYPE);
#else
		s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 1);
		s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
		s5p_usb_write_ep0_fifo((u8 *) &get_status, 1);
#endif
		otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GET_STATUS1:
#ifdef CONFIG_S5P_USB_DMA
		s5p_usb_setdma(((void *)&get_status) + 1, 1, CTRL_IN_TYPE);
#else
		s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 1);
		s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
		s5p_usb_write_ep0_fifo((u8 *) &get_status + 1, 1);
#endif
		otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GET_STATUS2:
#ifdef CONFIG_S5P_USB_DMA
		s5p_usb_setdma(((void *)&get_status) + 2, 1, CTRL_IN_TYPE);
#else
		s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 1);
		s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
		s5p_usb_write_ep0_fifo((u8 *) &get_status + 2, 1);
#endif
		otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GET_STATUS3:
#ifdef CONFIG_S5P_USB_DMA
		s5p_usb_setdma(((void *)&get_status) + 3, 1, CTRL_IN_TYPE);
#else
		s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 1);
		s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
		s5p_usb_write_ep0_fifo((u8 *) &get_status + 3, 1);
#endif
		otg.ep0_state = EP0_STATE_INIT;
		break;

	case EP0_STATE_GET_STATUS4:
#ifdef CONFIG_S5P_USB_DMA
		s5p_usb_setdma(((void *)&get_status) + 4, 1, CTRL_IN_TYPE);
#else
		s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, 1, 1);
		s5p_otg_write_reg(EPEN_CNAK_EP0_8, OTG_DIEPCTL0);
		s5p_usb_write_ep0_fifo((u8 *) &get_status + 4, 1);
#endif
		otg.ep0_state = EP0_STATE_INIT;
		break;

	default:
		break;
	}
}

void s5p_usb_tx(char *tx_data, int tx_size)
{
	otg.up_ptr = (u8 *) tx_data;
	otg.up_addr = (u32) tx_data;
	otg.up_size = tx_size;

	if (otg.op_mode == USB_CPU) {
		if (otg.up_size > otg.bulkin_max_pktsize) {
			s5p_usb_set_inep_xfersize(EP_TYPE_BULK, 1,
					otg.bulkin_max_pktsize);
		} else {
			s5p_usb_set_inep_xfersize(EP_TYPE_BULK, 1, otg.up_size);
		}

		/*ep1 enable, clear nak, bulk, usb active, max pkt */
		s5p_otg_write_reg(1u << 31 | 1 << 26 | 2 << 18 | 1 << 15 |
				otg.bulkin_max_pktsize << 0, OTG_DIEPCTL_IN);

		s5p_usb_write_in_fifo(otg.up_ptr, otg.up_size);
	} else if ((otg.op_mode == USB_DMA) && (otg.up_size >= 0)) {
#ifdef CONFIG_S5P_USB_DMA
		if (otg.up_size > (otg.bulkin_max_pktsize * 1023)) {
			s5p_usb_setdma((void *)otg.up_ptr, (otg.bulkin_max_pktsize * 1023), BULK_IN_TYPE);
		} else {
			s5p_usb_setdma((void *)otg.up_ptr, tx_size, BULK_IN_TYPE);
		}
#else
		u32 pktcnt, remainder;

		s5p_otg_write_reg(MODE_DMA | BURST_INCR4 | GBL_INT_UNMASK,
				OTG_GAHBCFG);
		s5p_otg_write_reg(INT_RESUME | INT_OUT_EP | INT_IN_EP |
				INT_ENUMDONE | INT_RESET | INT_SUSPEND,
				OTG_GINTMSK);

		s5p_otg_write_reg((u32) otg.up_ptr, OTG_DIEPDMA_IN);

		pktcnt = (u32) (otg.up_size / otg.bulkin_max_pktsize);
		remainder = (u32) (otg.up_size % otg.bulkin_max_pktsize);
		if (remainder != 0)
			pktcnt += 1;

		if (pktcnt > 1023) {
			s5p_usb_set_inep_xfersize(EP_TYPE_BULK, 1023,
					otg.bulkin_max_pktsize * 1023);
		} else {
			s5p_usb_set_inep_xfersize(EP_TYPE_BULK, pktcnt,
					otg.up_size);
		}

		/*ep1 enable, clear nak, bulk, usb active, next ep1, max pkt */
		s5p_otg_write_reg(1u << 31 | 1 << 26 | 2 << 18 | 1 << 15 |
				BULK_IN_EP << 11 | otg.bulkin_max_pktsize << 0,
				OTG_DIEPCTL_IN);
#endif
	}
}

static void s5p_usb_int_bulkout(u32 fifo_cnt_byte)
{
	s5p_usb_read_out_fifo((u8 *)otg.dn_ptr, fifo_cnt_byte);
	otg.dn_ptr += fifo_cnt_byte;

	s5p_usb_set_outep_xfersize(EP_TYPE_BULK, 1,
			otg.bulkout_max_pktsize);

	/*ep3 enable, clear nak, bulk, usb active, next ep3, max pkt */
	s5p_otg_write_reg(1u << 31 | 1 << 26 | 2 << 18 | 1 << 15 |
			otg.bulkout_max_pktsize << 0, OTG_DOEPCTL_OUT);

	if (((u32)otg.dn_ptr - otg.dn_addr) >= (otg.dn_filesize))
		s5p_receive_done = 1;
}

static void s5p_usb_dma_in_done(void)
{
	s32 remain_cnt;

	otg.up_ptr = (u8 *)s5p_otg_read_reg(OTG_DIEPDMA_IN);
	remain_cnt = otg.up_size - ((u32) otg.up_ptr - otg.up_addr);

#ifdef CONFIG_S5P_USB_DMA
	if (remain_cnt > 0) {
		if (remain_cnt > (otg.bulkin_max_pktsize * 1023)) {
			s5p_usb_setdma((void *)otg.up_ptr, (otg.bulkin_max_pktsize * 1023), BULK_IN_TYPE);
		} else {
			s5p_usb_setdma((void *)otg.up_ptr, remain_cnt, BULK_IN_TYPE);
		}
	}
#else
	if (remain_cnt > 0) {
		u32 pktcnt, remainder;
		pktcnt = (u32)(remain_cnt / otg.bulkin_max_pktsize);
		remainder = (u32)(remain_cnt % otg.bulkin_max_pktsize);

		if (remainder != 0)
			pktcnt += 1;

		if (pktcnt > 1023) {
			s5p_usb_set_inep_xfersize(EP_TYPE_BULK, 1023,
					otg.bulkin_max_pktsize * 1023);
		} else {
			s5p_usb_set_inep_xfersize(EP_TYPE_BULK, pktcnt,
					remain_cnt);
		}

		/*ep1 enable, clear nak, bulk, usb active, next ep1, max pkt */
		s5p_otg_write_reg(1u << 31 | 1 << 26 | 2 << 18 | 1 << 15 |
				BULK_IN_EP << 11 | otg.bulkin_max_pktsize << 0,
				OTG_DIEPCTL_IN);

		s5p_receive_done = 1;
	}
#endif
}

static void s5p_usb_dma_out_done(void)
{
	s32 remain_cnt;
	u32 aligned_ptr;

	otg.dn_ptr = (u8 *)s5p_otg_read_reg(OTG_DOEPDMA_OUT);
#ifdef CONFIG_S5P_USB_DMA
	/* sometimes host sends data which is not aligned size */
	aligned_ptr = ALIGN((u32)otg.dn_ptr, 16);
	otg.dn_addr += aligned_ptr - (u32)otg.dn_ptr;
	otg.dn_ptr = (u8 *)aligned_ptr;

	remain_cnt = otg.dn_filesize - ((u32) otg.dn_ptr - otg.dn_addr);

	if (remain_cnt > 0) {
		if (remain_cnt > (otg.bulkin_max_pktsize * 1023)) {
			s5p_usb_setdma((void *)otg.dn_ptr, (otg.bulkin_max_pktsize * 1023), BULK_OUT_TYPE);
		} else {
			s5p_usb_setdma((void *)otg.dn_ptr, remain_cnt, BULK_OUT_TYPE);
		}
	} else {
		s5p_receive_done = 1;
	}
#else
	remain_cnt = otg.dn_filesize - ((u32) otg.dn_ptr - otg.dn_addr + 8);

	if (remain_cnt > 0) {
		u32 pktcnt, remainder;
		pktcnt = (u32)(remain_cnt / otg.bulkout_max_pktsize);
		remainder = (u32)(remain_cnt % otg.bulkout_max_pktsize);

		if (remainder != 0)
			pktcnt += 1;

		if (pktcnt > 1023) {
			s5p_usb_set_outep_xfersize(EP_TYPE_BULK, 1023,
					otg.bulkout_max_pktsize * 1023);
		} else {
			s5p_usb_set_outep_xfersize(EP_TYPE_BULK, pktcnt,
					remain_cnt);
		}

		/*ep3 enable, clear nak, bulk, usb active, next ep3, max pkt 64 */
		s5p_otg_write_reg(1u << 31 | 1 << 26 | 2 << 18 | 1 << 15 |
				otg.bulkout_max_pktsize << 0, OTG_DOEPCTL_OUT);
	} else {
		udelay(500);	/*for FPGA ??? */
	}
#endif
}
#ifdef CONFIG_S5P_USB_DMA
void s5p_usb_dma_done(int direction) {
	if (direction)
		s5p_usb_dma_out_done();
	else
		s5p_usb_dma_in_done();
}
#endif
static void s5p_usb_set_all_outep_nak(int set)
{
	u8 i;
	u32 tmp;

	for (i = 0; i < 16; i++) {
		tmp = s5p_otg_read_reg(OTG_DOEPCTL0 + 0x20 * i);
		if (set)
			tmp |= DEPCTL_SNAK;
		else
			tmp |= (DEPCTL_EPENA | DEPCTL_CNAK);
		s5p_otg_write_reg(tmp, OTG_DOEPCTL0 + 0x20 * i);
	}
}

static void s5p_usb_set_max_pktsize(USB_SPEED speed)
{
	otg.speed = USB_HIGH;
	otg.ctrl_max_pktsize = HS_CTRL_PKT_SIZE;
	otg.bulkin_max_pktsize = HS_BULK_PKT_SIZE;
	otg.bulkout_max_pktsize = HS_BULK_PKT_SIZE;
}

static void s5p_usb_set_endpoint(void)
{
	/* Unmask OTG_DAINT source */
	s5p_otg_write_reg(0xff, OTG_DIEPINT0);
	s5p_otg_write_reg(0xff, OTG_DOEPINT0);
	s5p_otg_write_reg(0xff, OTG_DIEPINT_IN);
	s5p_otg_write_reg(0xff, OTG_DOEPINT_OUT);

#ifdef CONFIG_S5P_USB_DMA
	if (otg.speed == USB_HIGH) {
		/*MPS:64bytes */
		s5p_usb_setdma(usb_ctrl, 8, CTRL_OUT_TYPE);
	} else
#endif
	{
		/* Init For Ep0 */
		/*MPS:64bytes */
		s5p_otg_write_reg(((1 << 26) | (CONTROL_EP << 11) | (0 << 0)),
				OTG_DIEPCTL0);
		/*ep0 enable, clear nak */
		s5p_otg_write_reg((1u << 31) | (1 << 26) | (0 << 0),
				OTG_DOEPCTL0);
	}
}

static void s5p_usb_set_descriptors(void)
{
	/* Standard device descriptor */
	otg.desc.dev.bLength = DEVICE_DESC_SIZE;	/*0x12*/
	otg.desc.dev.bDescriptorType = DEVICE_DESCRIPTOR;
	otg.desc.dev.bDeviceClass = 0x02;
	otg.desc.dev.bDeviceSubClass = 0x02;	/*0x02:CDC-modem , 0x00:CDC-serial*/
	otg.desc.dev.bDeviceProtocol = 0x00;
	otg.desc.dev.bMaxPacketSize0 = otg.ctrl_max_pktsize;
	otg.desc.dev.idVendorL = 0xE8;	/*0x45;*/
	otg.desc.dev.idVendorH = 0x04;	/*0x53;*/
#ifdef CONFIG_USB_DEVGURU
	otg.desc.dev.idProductL = 0x60;
	otg.desc.dev.idProductH = 0x68;
#else
	otg.desc.dev.idProductL = 0x01; /*0x00*/
	otg.desc.dev.idProductH = 0x66; /*0x64*/
#endif
	otg.desc.dev.bcdDeviceL = 0x1B;
	otg.desc.dev.bcdDeviceH = 0x02;
	otg.desc.dev.iManufacturer = 0x1; /* index of string descriptor */
	otg.desc.dev.iProduct = 0x2;	/* index of string descriptor */
	otg.desc.dev.iSerialNumber = 0x0;
	otg.desc.dev.bNumConfigurations = 0x1;
	if (otg.speed == USB_FULL) {
		otg.desc.dev.bcdUSBL = 0x10;
		otg.desc.dev.bcdUSBH = 0x01;	/* Ver 1.10*/
	}
	else {
		otg.desc.dev.bcdUSBL = 0x00;
		otg.desc.dev.bcdUSBH = 0x02;	/* Ver 2.0*/
	}

	/* Standard configuration descriptor */
	otg.desc.config.bLength = CONFIG_DESC_SIZE;
	otg.desc.config.bDescriptorType = CONFIGURATION_DESCRIPTOR;
	otg.desc.config.wTotalLengthL = CONFIG_DESC_TOTAL_SIZE;
	otg.desc.config.wTotalLengthH = 0;
	otg.desc.config.bNumInterfaces = 0x02;
	otg.desc.config.bConfigurationValue = 0x02;
	otg.desc.config.iConfiguration = 0x00;
	otg.desc.config.bmAttributes = CONF_ATTR_DEFAULT|CONF_ATTR_SELFPOWERED;
	otg.desc.config.maxPower = 25;

	/* Standard interface descriptor */
	otg.desc.intf.bLength = INTERFACE_DESC_SIZE;
	otg.desc.intf.bDescriptorType = INTERFACE_DESCRIPTOR;
	otg.desc.intf.bInterfaceNumber = 0x0;
	otg.desc.intf.bAlternateSetting = 0x0;
	otg.desc.intf.bNumEndpoints = 0x01;
	otg.desc.intf.bInterfaceClass = 0x02;
	otg.desc.intf.bInterfaceSubClass = 0x02;
	otg.desc.intf.bInterfaceProtocol = 0x01;
	otg.desc.intf.iInterface = 0x00;



	otg.desc.cdc_header.bLength = 0x05;
	otg.desc.cdc_header.bDescriptorType = 0x24;
	otg.desc.cdc_header.bDescriptorSubType = 0x00;
	otg.desc.cdc_header.bcdCDC = 0x0110;

	otg.desc.cdc_call.bLength = 0x05;
	otg.desc.cdc_call.bDescriptorType = 0x24;
	otg.desc.cdc_call.bDescriptorSubType = 0x01;
	otg.desc.cdc_call.bmCapabilities = 0x00;
	otg.desc.cdc_call.bDataInterface = 0x01;

	otg.desc.cdc_abstract.bLength = 0x04;
	otg.desc.cdc_abstract.bDescriptorType = 0x24;
	otg.desc.cdc_abstract.bDescriptorSubType = 0x02;
	/*old value is bmCapabilities = 0x0F , but now we doesn't support it*/
	otg.desc.cdc_abstract.bmCapabilities = 0x00;

	otg.desc.cdc_union.bLength = 0x05;
	otg.desc.cdc_union.bDescriptorType = 0x24;
	otg.desc.cdc_union.bDescriptorSubType = 0x06;
	otg.desc.cdc_union.bMasterInterface0 = 0x00;
	otg.desc.cdc_union.bSlaveInterface0 = 0x01;

	/* Standard endpoint0 descriptor */
	otg.desc.ep3.bLength = ENDPOINT_DESC_SIZE;
	otg.desc.ep3.bDescriptorType = ENDPOINT_DESCRIPTOR;
	otg.desc.ep3.bEndpointAddress = 3 | EP_ADDR_IN;
	otg.desc.ep3.bmAttributes = EP_ATTR_INTERRUPT;
	otg.desc.ep3.wMaxPacketSizeL = 0x10;
	otg.desc.ep3.wMaxPacketSizeH = 0x00;
	otg.desc.ep3.bInterval = 0x9; /* not used */

	/* Standard interface descriptor */
	otg.desc.intf2.bLength = INTERFACE_DESC_SIZE; /* 9*/
	otg.desc.intf2.bDescriptorType = INTERFACE_DESCRIPTOR;
	otg.desc.intf2.bInterfaceNumber = 0x1;
	otg.desc.intf2.bAlternateSetting = 0x0; /* ?*/
	otg.desc.intf2.bNumEndpoints = 0x02;	/* # of endpoints except EP0*/
	otg.desc.intf2.bInterfaceClass = 0x0A; /* 0x0 ?*/
	otg.desc.intf2.bInterfaceSubClass = 0x00;
	otg.desc.intf2.bInterfaceProtocol = 0x00; /*0x00:CDC-modem , 0xFF:CDC-serial*/
	otg.desc.intf2.iInterface = 0x00;
#ifdef CONFIG_S5P_USB_NON_ZLP
	/* Attirbuted Vendo descriptor for NON-ZLP*/
	otg.desc.avd.bLength = 0x08;
	otg.desc.avd.bDescriptorType = 0x24;
	otg.desc.avd.bDescriptorSubType = 0x80;
	otg.desc.avd.DAUType = 0x0002;
	otg.desc.avd.DAULength = 0x0001;
	otg.desc.avd.DAUValue = 0x00;
#endif
	/* Standard endpoint0 descriptor */
	otg.desc.ep1.bLength = ENDPOINT_DESC_SIZE;
	otg.desc.ep1.bDescriptorType = ENDPOINT_DESCRIPTOR;
	otg.desc.ep1.bEndpointAddress = BULK_IN_EP | EP_ADDR_IN;
	otg.desc.ep1.bmAttributes = EP_ATTR_BULK;
	otg.desc.ep1.wMaxPacketSizeL = (u8)otg.bulkin_max_pktsize; /* 64*/
	otg.desc.ep1.wMaxPacketSizeH = (u8)(otg.bulkin_max_pktsize>>8);
	otg.desc.ep1.bInterval = 0x0; /* not used */

	/* Standard endpoint1 descriptor */
	otg.desc.ep2.bLength = ENDPOINT_DESC_SIZE;
	otg.desc.ep2.bDescriptorType = ENDPOINT_DESCRIPTOR;
	otg.desc.ep2.bEndpointAddress = BULK_OUT_EP|EP_ADDR_OUT;
	//otg.desc.ep2.bEndpointAddress = 3 | EP_ADDR_OUT;
	otg.desc.ep2.bmAttributes = EP_ATTR_BULK;
	otg.desc.ep2.wMaxPacketSizeL = (u8)otg.bulkout_max_pktsize; /* 64*/
	otg.desc.ep2.wMaxPacketSizeH = (u8)(otg.bulkout_max_pktsize>>8);
	otg.desc.ep2.bInterval = 0x0; /* not used */
}

static void s5p_usb_set_opmode(USB_OPMODE mode)
{
	otg.op_mode = mode;

#ifdef CONFIG_S5P_USB_DMA
	s5p_otg_write_reg(INT_RESUME | INT_OUT_EP | INT_IN_EP | INT_ENUMDONE |
			INT_RESET | INT_SUSPEND ,
			OTG_GINTMSK);

	s5p_otg_write_reg(PTXFE_HALF | NPTXFE_HALF | MODE_DMA | BURST_INCR4 | GBL_INT_UNMASK,
			OTG_GAHBCFG);
#else
	s5p_otg_write_reg(INT_RESUME | INT_OUT_EP | INT_IN_EP | INT_ENUMDONE |
			INT_RESET | INT_SUSPEND | INT_RX_FIFO_NOT_EMPTY,
			OTG_GINTMSK);

	s5p_otg_write_reg(MODE_SLAVE | BURST_SINGLE | GBL_INT_UNMASK,
			OTG_GAHBCFG);

	s5p_usb_set_outep_xfersize(EP_TYPE_BULK, 1, otg.bulkout_max_pktsize);
	s5p_usb_set_inep_xfersize(EP_TYPE_BULK, 1, 0);

	/*bulk out ep enable, clear nak, bulk, usb active, next ep3, max pkt */
	s5p_otg_write_reg(1u << 31 | 1 << 26 | 2 << 18 | 1 << 15 |
			otg.bulkout_max_pktsize << 0, OTG_DOEPCTL_OUT);

	/*bulk in ep enable, clear nak, bulk, usb active, next ep1, max pkt */
	s5p_otg_write_reg(0u << 31 | 1 << 26 | 2 << 18 | 1 << 15 |
			otg.bulkin_max_pktsize << 0, OTG_DIEPCTL_IN);
#endif
}

static void s5p_usb_reset(void)
{
	s5p_usb_set_all_outep_nak(1);

	otg.ep0_state = EP0_STATE_INIT;
	s5p_otg_write_reg(((1 << BULK_OUT_EP) | (1 << CONTROL_EP)) << 16 |
			((1 << BULK_IN_EP) | (1 << CONTROL_EP)), OTG_DAINTMSK);
	s5p_otg_write_reg(CTRL_OUT_EP_SETUP_PHASE_DONE | AHB_ERROR |
			TRANSFER_DONE, OTG_DOEPMSK);
	s5p_otg_write_reg(INTKN_TXFEMP | NON_ISO_IN_EP_TIMEOUT | AHB_ERROR |
			TRANSFER_DONE, OTG_DIEPMSK);

	/* Rx FIFO Size */
	s5p_otg_write_reg(RX_FIFO_SIZE, OTG_GRXFSIZ);

	/* Non Periodic Tx FIFO Size */
	s5p_otg_write_reg(NPTX_FIFO_SIZE << 16 | NPTX_FIFO_START_ADDR << 0,
			OTG_GNPTXFSIZ);

#ifdef CONFIG_S5P_USB_DMA
	s5p_otg_write_reg(PTX_FIFO_SIZE<<16|(NPTX_FIFO_START_ADDR+NPTX_FIFO_SIZE)<<0, OTG_DPTXFSIZ1);
#endif

	s5p_usb_set_all_outep_nak(0);

	/*clear device address */
	s5p_otg_write_reg(s5p_otg_read_reg(OTG_DCFG) & ~(0x7f << 4),
			OTG_DCFG);

	s5p_usb_connected = 0;
}

static int s5p_usb_set_init(void)
{
	u32 status;

	status = s5p_otg_read_reg(OTG_DSTS);

	/* Set if Device is High speed or Full speed */
	if (((status & 0x6) >> 1) == USB_HIGH) {
		s5p_usb_set_max_pktsize(USB_HIGH);
	} else if (((status & 0x6) >> 1) == USB_FULL) {
		s5p_usb_set_max_pktsize(USB_FULL);
	} else {
		puts("Error: Neither High_Speed nor Full_Speed\n");
		return 0;
	}

	s5p_usb_set_endpoint();
	s5p_usb_set_descriptors();
	s5p_usb_set_opmode(op_mode);

	return 1;
}

#ifndef CONFIG_S5P_USB_DMA
static void read_control_fifo(void)
{
	int i;
	u32 fifo = otg_base + OTG_EP0_FIFO;
	u32 tmp_buf;

	for (i = 0; i < 2; i++)
		tmp_buf = readl(fifo);
}

static void s5p_usb_pkt_receive(void)
{
	u32 rx_status;
	u32 fifo_cnt_byte;

	rx_status = s5p_otg_read_reg(OTG_GRXSTSP);

	if ((rx_status & (0xf << 17)) == SETUP_PKT_RECEIVED) {
		s5p_usb_ep0_int_hndlr();
	} else if ((rx_status & (0xf << 17)) == OUT_PKT_RECEIVED) {
		fifo_cnt_byte = (rx_status & 0x7ff0) >> 4;

		/* must need to support downloader */
		if(((rx_status & INTR_IN_EP) == 0) && fifo_cnt_byte)
			read_control_fifo();

		if (rx_status & BULK_OUT_EP) {
			if (fifo_cnt_byte) {
				s5p_usb_int_bulkout(fifo_cnt_byte);
				if (otg.op_mode == USB_CPU) {
					s5p_otg_write_reg(INT_RESUME | INT_OUT_EP |
						INT_IN_EP | INT_ENUMDONE | INT_RESET |
						INT_SUSPEND | INT_RX_FIFO_NOT_EMPTY,
						OTG_GINTMSK);
				}
			} else {
				s5p_usb_set_outep_xfersize(EP_TYPE_BULK, 1,
						otg.bulkout_max_pktsize);
				/*ep3 enable, clear nak, bulk, usb active, next ep3, max pkt */
				s5p_otg_write_reg(1u << 31 | 1 << 26 | 2 << 18 | 1 << 15 |
						otg.bulkout_max_pktsize << 0, OTG_DOEPCTL_OUT);
			}
		}
	}
}
#endif

static void s5p_usb_transfer(void)
{
	u32 ep_int;
	u32 check_dma;
	u32 ep_int_status;

	ep_int = s5p_otg_read_reg(OTG_DAINT);

	if (ep_int & (1 << CONTROL_EP)) {
		ep_int_status = s5p_otg_read_reg(OTG_DIEPINT0);

#ifdef CONFIG_S5P_USB_DMA
		if(ep_int_status & TRANSFER_DONE) {
			s5p_usb_setdma(usb_ctrl, 8, CTRL_OUT_TYPE);
		}
#else
		if (ep_int_status & INTKN_TXFEMP) {
			u32 uNTxFifoSpace;
			do {
				uNTxFifoSpace = s5p_otg_read_reg(OTG_GNPTXSTS)
						& 0xffff;
			} while (uNTxFifoSpace < otg.ctrl_max_pktsize);

			s5p_usb_transfer_ep0();
		}
#endif
		s5p_otg_write_reg(ep_int_status, OTG_DIEPINT0);
	}

	if (ep_int & ((1 << CONTROL_EP) << 16)) {
		ep_int_status = s5p_otg_read_reg(OTG_DOEPINT0);

#ifdef CONFIG_S5P_USB_DMA
		if(ep_int_status & CTRL_OUT_EP_SETUP_PHASE_DONE) {
			s5p_usb_ep0_int_hndlr();
		}

		if(ep_int_status & TRANSFER_DONE) {
			s5p_usb_setdma(usb_ctrl, 8, CTRL_OUT_TYPE);
		}
#else
		s5p_usb_set_outep_xfersize(EP_TYPE_CONTROL, 1, 8);
		s5p_otg_write_reg(1u << 31 | 1 << 26, OTG_DOEPCTL0);
#endif
		s5p_otg_write_reg(ep_int_status, OTG_DOEPINT0);
	}

	if (ep_int & (1 << BULK_IN_EP)) {
		ep_int_status = s5p_otg_read_reg(OTG_DIEPINT_IN);

		s5p_otg_write_reg(ep_int_status, OTG_DIEPINT_IN);
		check_dma = s5p_otg_read_reg(OTG_GAHBCFG);
#ifdef CONFIG_S5P_USB_DMA
		if ((check_dma & MODE_DMA) && (ep_int_status & TRANSFER_DONE))
			s5p_usb_dma_in_done();
#endif
	}

	if (ep_int & ((1 << BULK_OUT_EP) << 16)) {
		ep_int_status = s5p_otg_read_reg(OTG_DOEPINT_OUT);

		s5p_otg_write_reg(ep_int_status, OTG_DOEPINT_OUT);
		check_dma = s5p_otg_read_reg(OTG_GAHBCFG);
#ifdef CONFIG_S5P_USB_DMA
		if ((check_dma & MODE_DMA) && (ep_int_status & TRANSFER_DONE))
			s5p_usb_dma_out_done();
#endif
	}
}

void s5p_udc_int_hndlr(void)
{
	u32 int_status;
	int tmp;

	int_status = s5p_otg_read_reg(OTG_GINTSTS);
	s5p_otg_write_reg(int_status, OTG_GINTSTS);

	if (int_status & INT_RESET) {
		s5p_otg_write_reg(INT_RESET, OTG_GINTSTS);
		s5p_usb_reset();
	}

	if (int_status & INT_ENUMDONE) {
		s5p_otg_write_reg(INT_ENUMDONE, OTG_GINTSTS);

		tmp = s5p_usb_set_init();
		if (tmp == 0)
			return;
	}

	if (int_status & INT_RESUME)
		s5p_otg_write_reg(INT_RESUME, OTG_GINTSTS);

	if (int_status & INT_SUSPEND)
		s5p_otg_write_reg(INT_SUSPEND, OTG_GINTSTS);

#ifndef CONFIG_S5P_USB_DMA
	if (int_status & INT_RX_FIFO_NOT_EMPTY) {
		s5p_otg_write_reg(INT_RESUME | INT_OUT_EP | INT_IN_EP |
				INT_ENUMDONE | INT_RESET | INT_SUSPEND,
				OTG_GINTMSK);

		s5p_usb_pkt_receive();

		s5p_otg_write_reg(INT_RESUME | INT_OUT_EP | INT_IN_EP |
				INT_ENUMDONE | INT_RESET | INT_SUSPEND |
				INT_RX_FIFO_NOT_EMPTY, OTG_GINTMSK);
	}
#endif

	if ((int_status & INT_IN_EP) || (int_status & INT_OUT_EP))
		s5p_usb_transfer();
}

void s5p_usb_set_dn_addr(unsigned long addr, unsigned long size)
{
	return;
}

#ifdef CONFIG_S5P_USB_DMA
void s5p_usb_setdma(void *addr , u32 bytes , uchar type)
{
	u32 pktcnt;
	u32 buf_addrs = (u32)addr;
	u32 status;

	switch (type)
	{
		case BULK_RESET_DATA_PID:
			status = s5p_otg_read_reg(OTG_DOEPCTL_OUT);
			s5p_otg_write_reg(1u << 28 | 1 << 26 | status, OTG_DOEPCTL_OUT);

			status = s5p_otg_read_reg(OTG_DIEPCTL_IN);
			s5p_otg_write_reg(1u << 28 | 1 << 26 | status, OTG_DIEPCTL_IN);
			break;

		case CTRL_IN_TYPE:
			if ( buf_addrs & 0x7 ) {
				memcpy(ctrl_buf, addr, bytes);
				addr = (void *)ctrl_buf;
			}
			/*change DMA addr point*/
			s5p_otg_write_reg(virt_to_phys(addr), OTG_DIEPDMA0);

			if(bytes == 0)
				pktcnt = 1;
			else
				pktcnt = (bytes - 1) / 64 + 1;

			s5p_usb_set_inep_xfersize(EP_TYPE_CONTROL, pktcnt, bytes);
			/*ep0 enable, clear nak */
			status = s5p_otg_read_reg(OTG_DIEPCTL0);
			s5p_otg_write_reg(1u << 31 | 1 << 26 | status, OTG_DIEPCTL0);
			break;

		case CTRL_OUT_TYPE:
			s5p_otg_write_reg(virt_to_phys(addr), OTG_DOEPDMA0);
			s5p_usb_set_outep_xfersize(EP_TYPE_CONTROL, 1, bytes);

			status = s5p_otg_read_reg(OTG_DOEPCTL0);
			s5p_otg_write_reg(1u << 31 | 1 << 26 | status, OTG_DOEPCTL0);
			break;

		case BULK_IN_TYPE:
			if ( buf_addrs & 0x7 ) {
				if ( bytes > HS_BULK_PKT_SIZE ) {
					printf("\r|s5p_usb_setdma(BULK_IN): tmp retry error, overflow tmp buffer : %d\n",bytes);
					return;
				} else {
					memcpy(bulk_in_tmp, addr, bytes);
					otg.up_ptr = (u8 *) bulk_in_tmp;
					otg.up_addr = (u32) bulk_in_tmp;
					otg.up_size = bytes;
					s5p_otg_write_reg(virt_to_phys(bulk_in_tmp), OTG_DIEPDMA_IN);
				}
			} else {
				/*change DMA addr point*/
				s5p_otg_write_reg(virt_to_phys(addr), OTG_DIEPDMA_IN);
			}

			if(bytes == 0)
				pktcnt = 1;
			else
				pktcnt = (bytes - 1) / HS_BULK_PKT_SIZE + 1;

			s5p_usb_set_inep_xfersize(EP_TYPE_BULK, pktcnt, bytes);
			/*ep0 enable, clear nak */
			status = s5p_otg_read_reg(OTG_DIEPCTL_IN);
			s5p_otg_write_reg(1u << 31 | 1 << 26 | status, OTG_DIEPCTL_IN);
			break;

		case BULK_OUT_TYPE:
			if ( buf_addrs & 0x7 ) {
				printf("\r|s5p_usb_setdma(BULK_OUT): error !! DMA address is not aligned by 8----\n");
				return;
			}
			/*change DMA addr point*/
			s5p_otg_write_reg(virt_to_phys(addr), OTG_DOEPDMA_OUT);

			if(bytes == 0)
				pktcnt = 1;
			else
				pktcnt = (bytes - 1) / HS_BULK_PKT_SIZE + 1;

			s5p_usb_set_outep_xfersize(EP_TYPE_BULK, pktcnt, bytes);
			/*ep0 enable, clear nak */
			status = s5p_otg_read_reg(OTG_DOEPCTL_OUT);
			s5p_otg_write_reg(1u << 31 | 1 << 26 | status, OTG_DOEPCTL_OUT);
			break;

		default:
			printf("\r|s5p_usb_setdma: error !! Does not supported type : %d----\n",type);
			break;
	}
}
#endif
