/*
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 * Minkyu Kang <mk7.kang@samsung.com>
 * Sanghee Kim <sh0130.kim@samsung.com>
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

#ifndef __ASM_ARM_ARCH_CLOCK_H_
#define __ASM_ARM_ARCH_CLOCK_H_

#ifndef __ASSEMBLY__
struct exynos4_clock {
#if defined(CONFIG_EXYNOS4210)
	unsigned char	res1[0x4200];
	unsigned int	src_leftbus;
	unsigned char	res2[0x1fc];
	unsigned int	mux_stat_leftbus;
	unsigned char	res4[0xfc];
	unsigned int	div_leftbus;
	unsigned char	res5[0xfc];
	unsigned int	div_stat_leftbus;
	unsigned char	res6[0x1fc];
	unsigned int	gate_ip_leftbus;
	unsigned char	res7[0x1fc];
	unsigned int	clkout_leftbus;
	unsigned int	clkout_leftbus_div_stat;
	unsigned char	res8[0x37f8];
	unsigned int	src_rightbus;
	unsigned char	res9[0x1fc];
	unsigned int	mux_stat_rightbus;
	unsigned char	res10[0xfc];
	unsigned int	div_rightbus;
	unsigned char	res11[0xfc];
	unsigned int	div_stat_rightbus;
	unsigned char	res12[0x1fc];
	unsigned int	gate_ip_rightbus;
	unsigned char	res13[0x1fc];
	unsigned int	clkout_rightbus;
	unsigned int	clkout_rightbus_div_stat;
	unsigned char	res14[0x3608];
	unsigned int	epll_lock;
	unsigned char	res15[0xc];
	unsigned int	vpll_lock;
	unsigned char	res16[0xec];
	unsigned int	epll_con0;
	unsigned int	epll_con1;
	unsigned char	res17[0x8];
	unsigned int	vpll_con0;
	unsigned int	vpll_con1;
	unsigned char	res18[0xe8];
	unsigned int	src_top0;
	unsigned int	src_top1;
	unsigned char	res19[0x8];
	unsigned int	src_cam;
	unsigned int	src_tv;
	unsigned int	src_mfc;
	unsigned int	src_g3d;
	unsigned int	src_image;
	unsigned int	src_lcd0;
	unsigned int	src_lcd1;
	unsigned int	src_maudio;
	unsigned int	src_fsys;
	unsigned char	res20[0xc];
	unsigned int	src_peril0;
	unsigned int	src_peril1;
	unsigned char	res21[0xb8];
	unsigned int	src_mask_top;
	unsigned char	res22[0xc];
	unsigned int	src_mask_cam;
	unsigned int	src_mask_tv;
	unsigned char	res23[0xc];
	unsigned int	src_mask_lcd0;
	unsigned int	src_mask_lcd1;
	unsigned int	src_mask_maudio;
	unsigned int	src_mask_fsys;
	unsigned char	res24[0xc];
	unsigned int	src_mask_peril0;
	unsigned int	src_mask_peril1;
	unsigned char	res25[0xb8];
	unsigned int	mux_stat_top;
	unsigned char	res26[0x14];
	unsigned int	mux_stat_mfc;
	unsigned int	mux_stat_g3d;
	unsigned int	mux_stat_image;
	unsigned char	res27[0xdc];
	unsigned int	div_top;
	unsigned char	res28[0xc];
	unsigned int	div_cam;
	unsigned int	div_tv;
	unsigned int	div_mfc;
	unsigned int	div_g3d;
	unsigned int	div_image;
	unsigned int	div_lcd0;
	unsigned int	div_lcd1;
	unsigned int	div_maudio;
	unsigned int	div_fsys0;
	unsigned int	div_fsys1;
	unsigned int	div_fsys2;
	unsigned int	div_fsys3;
	unsigned int	div_peril0;
	unsigned int	div_peril1;
	unsigned int	div_peril2;
	unsigned int	div_peril3;
	unsigned int	div_peril4;
	unsigned int	div_peril5;
	unsigned char	res29[0x18];
	unsigned int	div2_ratio;
	unsigned char	res30[0x8c];
	unsigned int	div_stat_top;
	unsigned char	res31[0xc];
	unsigned int	div_stat_cam;
	unsigned int	div_stat_tv;
	unsigned int	div_stat_mfc;
	unsigned int	div_stat_g3d;
	unsigned int	div_stat_image;
	unsigned int	div_stat_lcd0;
	unsigned int	div_stat_lcd1;
	unsigned int	div_stat_maudio;
	unsigned int	div_stat_fsys0;
	unsigned int	div_stat_fsys1;
	unsigned int	div_stat_fsys2;
	unsigned int	div_stat_fsys3;
	unsigned int	div_stat_peril0;
	unsigned int	div_stat_peril1;
	unsigned int	div_stat_peril2;
	unsigned int	div_stat_peril3;
	unsigned int	div_stat_peril4;
	unsigned int	div_stat_peril5;
	unsigned char	res32[0x18];
	unsigned int	div2_stat;
	unsigned char	res33[0x29c];
	unsigned int	gate_ip_cam;
	unsigned int	gate_ip_tv;
	unsigned int	gate_ip_mfc;
	unsigned int	gate_ip_g3d;
	unsigned int	gate_ip_image;
	unsigned int	gate_ip_lcd0;
	unsigned int	gate_ip_lcd1;
	unsigned char	res34[0x4];
	unsigned int	gate_ip_fsys;
	unsigned char	res35[0x8];
	unsigned int	gate_ip_gps;
	unsigned int	gate_ip_peril;
	unsigned char	res36[0xc];
	unsigned int	gate_ip_perir;
	unsigned char	res37[0xc];
	unsigned int	gate_block;
	unsigned char	res38[0x8c];
	unsigned int	clkout_cmu_top;
	unsigned int	clkout_cmu_top_div_stat;
	unsigned char	res39[0x37f8];
	unsigned int	src_dmc;
	unsigned char	res40[0xfc];
	unsigned int	src_mask_dmc;
	unsigned char	res41[0xfc];
	unsigned int	mux_stat_dmc;
	unsigned char	res42[0xfc];
	unsigned int	div_dmc0;
	unsigned int	div_dmc1;
	unsigned char	res43[0xf8];
	unsigned int	div_stat_dmc0;
	unsigned int	div_stat_dmc1;
	unsigned char	res44[0x2f8];
	unsigned int	gate_ip_dmc;
	unsigned char	res45[0xfc];
	unsigned int	clkout_cmu_dmc;
	unsigned int	clkout_cmu_dmc_div_stat;
	unsigned char	res46[0x5f8];
	unsigned int	dcgidx_map0;
	unsigned int	dcgidx_map1;
	unsigned int	dcgidx_map2;
	unsigned char	res47[0x14];
	unsigned int	dcgperf_map0;
	unsigned int	dcgperf_map1;
	unsigned char	res48[0x18];
	unsigned int	dvcidx_map;
	unsigned char	res49[0x1c];
	unsigned int	freq_cpu;
	unsigned int	freq_dpm;
	unsigned char	res50[0x18];
	unsigned int	dvsemclk_en;
	unsigned int	maxperf;
	unsigned char	res51[0x2f78];
	unsigned int	apll_lock;
	unsigned char	res52[0x4];
	unsigned int	mpll_lock;
	unsigned char	res53[0xf4];
	unsigned int	apll_con0;
	unsigned int	apll_con1;
	unsigned int	mpll_con0;
	unsigned int	mpll_con1;
	unsigned char	res54[0xf0];
	unsigned int	src_cpu;
	unsigned char	res55[0x1fc];
	unsigned int	mux_stat_cpu;
	unsigned char	res56[0xfc];
	unsigned int	div_cpu0;
	unsigned int	div_cpu1;
	unsigned char	res57[0xf8];
	unsigned int	div_stat_cpu0;
	unsigned int	div_stat_cpu1;
	unsigned char	res58[0x3f8];
	unsigned int	clkout_cmu_cpu;
	unsigned int	clkout_cmu_cpu_div_stat;
	unsigned char	res59[0x5f8];
	unsigned int	armclk_stopctrl;
	unsigned int	atclk_stopctrl;
	unsigned char	res60[0x8];
	unsigned int	parityfail_status;
	unsigned int	parityfail_clear;
	unsigned char	res61[0xe8];
	unsigned int	apll_con0_l8;
	unsigned int	apll_con0_l7;
	unsigned int	apll_con0_l6;
	unsigned int	apll_con0_l5;
	unsigned int	apll_con0_l4;
	unsigned int	apll_con0_l3;
	unsigned int	apll_con0_l2;
	unsigned int	apll_con0_l1;
	unsigned int	iem_control;
	unsigned char	res62[0xdc];
	unsigned int	apll_con1_l8;
	unsigned int	apll_con1_l7;
	unsigned int	apll_con1_l6;
	unsigned int	apll_con1_l5;
	unsigned int	apll_con1_l4;
	unsigned int	apll_con1_l3;
	unsigned int	apll_con1_l2;
	unsigned int	apll_con1_l1;
	unsigned char	res63[0xe0];
	unsigned int	div_iem_l8;
	unsigned int	div_iem_l7;
	unsigned int	div_iem_l6;
	unsigned int	div_iem_l5;
	unsigned int	div_iem_l4;
	unsigned int	div_iem_l3;
	unsigned int	div_iem_l2;
	unsigned int	div_iem_l1;
#else
	unsigned char	res1[0x4200];
	unsigned int	src_leftbus;			/* 0x4200 */
	unsigned char	res2[0x1fc];
	unsigned int	mux_stat_leftbus;		/* 0x4400 */
	unsigned char	res4[0xfc];
	unsigned int	div_leftbus;			/* 0x4500 */
	unsigned char	res5[0xfc];
	unsigned int	div_stat_leftbus;		/* 0x4600 */
	unsigned char	res6[0x1fc];
	unsigned int	gate_ip_leftbus;		/* 0x4800 */
	unsigned char	res7[0x1fc];
	unsigned int	clkout_leftbus;			/* 0x4a00 */
	unsigned int	clkout_leftbus_div_stat;	/* 0x4a04 */
	unsigned char	res8[0x37f8];
	unsigned int	src_rightbus;			/* 0x8200 */
	unsigned char	res9[0x1fc];
	unsigned int	mux_stat_rightbus;		/* 0x8400 */
	unsigned char	res10[0xfc];
	unsigned int	div_rightbus;			/* 0x8500 */
	unsigned char	res11[0xfc];
	unsigned int	div_stat_rightbus;		/* 0x8600 */
	unsigned char	res12[0x1fc];
	unsigned int	gate_ip_rightbus;		/* 0x8800 */
	unsigned char	res13[0x1fc];
	unsigned int	clkout_rightbus;		/* 0x8a00 */
	unsigned int	clkout_rightbus_div_stat;	/* 0x8a04 */
	unsigned char	res14[0x3608];
	unsigned int	epll_lock;			/* 0xc010 */
	unsigned char	res15[0xc];
	unsigned int	vpll_lock;			/* 0xc020 */
	unsigned char	res16[0xec];
	unsigned int	epll_con0;			/* 0xc110 */
	unsigned int	epll_con1;			/* 0xc114 */
	unsigned int	epll_con2;			/* 0xc118 */
	unsigned char	res17[0x4];
	unsigned int	vpll_con0;			/* 0xc120 */
	unsigned int	vpll_con1;			/* 0xc124 */
	unsigned int	vpll_con2;			/* 0xc128 */
	unsigned char	res18[0xe4];
	unsigned int	src_top0;			/* 0xc210 */
	unsigned int	src_top1;			/* 0xc214 */
	unsigned char	res19[0x8];
	unsigned int	src_cam0;			/* 0xc220 */
	unsigned int	src_tv;				/* 0xc224 */
	unsigned int	src_mfc;			/* 0xc228 */
	unsigned int	src_g3d;			/* 0xc22c */
	unsigned int	res20;
	unsigned int	src_lcd;			/* 0xc234 */
	unsigned int	src_isp;			/* 0xc238 */
	unsigned int	src_maudio;			/* 0xc23c */
	unsigned int	src_fsys;			/* 0xc240 */
	unsigned char	res21[0xc];
	unsigned int	src_peril0;			/* 0xc250 */
	unsigned int	src_peril1;			/* 0xc254 */
	unsigned int	src_cam1;			/* 0xc258 */
	unsigned char	res22[0xb4];
	unsigned int	src_mask_top;			/* 0xc310 */
	unsigned char	res23[0xc];
	unsigned int	src_mask_cam0;			/* 0xc320 */
	unsigned int	src_mask_tv;			/* 0xc324 */
	unsigned char	res24[0xc];
	unsigned int	src_mask_lcd;			/* 0xc334 */
	unsigned int	src_mask_isp;			/* 0xc338 */
	unsigned int	src_mask_maudio;		/* 0xc33c */
	unsigned int	src_mask_fsys;			/* 0xc340 */
	unsigned char	res25[0xc];
	unsigned int	src_mask_peril0;		/* 0xc350 */
	unsigned int	src_mask_peril1;		/* 0xc354 */
	unsigned char	res26[0xb8];
	unsigned int	mux_stat_top0;			/* 0xc410 */
	unsigned int	mux_stat_top1;			/* 0xc414 */
	unsigned char	res27[0x10];
	unsigned int	mux_stat_mfc;			/* 0xc428 */
	unsigned int	mux_stat_g3d;			/* 0xc42c */
	unsigned char	res28[0x28];
	unsigned int	mux_stat_cam1;			/* 0xc458 */
	unsigned char	res29[0xb4];
	unsigned int	div_top;			/* 0xc510 */
	unsigned char	res30[0xc];
	unsigned int	div_cam0;			/* 0xc520 */
	unsigned int	div_tv;
	unsigned int	div_mfc;
	unsigned int	div_g3d;
	unsigned int	res31;				/* 0xc530 */
	unsigned int	div_lcd;
	unsigned int	div_isp;
	unsigned int	div_maudio;
	unsigned int	div_fsys0;			/* 0xc540 */
	unsigned int	div_fsys1;
	unsigned int	div_fsys2;
	unsigned int	div_fsys3;
	unsigned int	div_peril0;			/* 0xc550 */
	unsigned int	div_peril1;
	unsigned int	div_peril2;
	unsigned int	div_peril3;
	unsigned int	div_peril4;			/* 0xc560 */
	unsigned int	div_peril5;			/* 0xc564 */
	unsigned int	div_cam1;			/* 0xc568 */
	unsigned char	res32[0x14];
	unsigned int	div2_ratio;			/* 0xc580 */
	unsigned char	res33[0x8c];
	unsigned int	div_stat_top;			/* 0xc610 */
	unsigned char	res34[0xc];
	unsigned int	div_stat_cam0;			/* 0xc620 */
	unsigned int	div_stat_tv;
	unsigned int	div_stat_mfc;
	unsigned int	div_stat_g3d;
	unsigned int	res35;				/* 0xc630 */
	unsigned int	div_stat_lcd;
	unsigned int	div_stat_isp;
	unsigned int	div_stat_maudio;
	unsigned int	div_stat_fsys0;			/* 0xc640 */
	unsigned int	div_stat_fsys1;
	unsigned int	div_stat_fsys2;
	unsigned int	div_stat_fsys3;
	unsigned int	div_stat_peril0;		/* 0xc650 */
	unsigned int	div_stat_peril1;
	unsigned int	div_stat_peril2;
	unsigned int	div_stat_peril3;
	unsigned int	div_stat_peril4;		/* 0xc660 */
	unsigned int	div_stat_peril5;
	unsigned int	div_stat_cam1;			/* 0xc668 */
	unsigned char	res36[0x14];
	unsigned int	div2_stat;			/* 0xc680 */
	unsigned char	res37[0x29c];
	unsigned int	gate_ip_cam;			/* 0xc920 */
	unsigned int	gate_ip_tv;
	unsigned int	gate_ip_mfc;
	unsigned int	gate_ip_g3d;
	unsigned int	res38;				/* 0xc930 */
	unsigned int	gate_ip_lcd;
	unsigned int	gate_ip_isp;
	unsigned int	gate_ip_maudio;
	unsigned int	gate_ip_fsys;			/* 0xc940 */
	unsigned char	res39[0x8];
	unsigned int	gate_ip_gps;			/* 0xc94c */
	unsigned int	gate_ip_peril;			/* 0xc950 */
	unsigned char	res40[0xc];
	unsigned int	gate_ip_perir;			/* 0xc960 */
	unsigned char	res41[0xc];
	unsigned int	gate_block;			/* 0xc970 */
	unsigned char	res42[0x8c];
	unsigned int	clkout_cmu_top;			/* 0xca00 */
	unsigned int	clkout_cmu_top_div_stat;	/* 0xca04 */
	unsigned char	res43[0x35f8];
	unsigned char	res44[0x8];			/* 0x10000 */
	unsigned int	mpll_lock;			/* 0x10008 */
	unsigned char	res45[0xfc];
	unsigned int	mpll_con0;			/* 0x10108 */
	unsigned int	mpll_con1;			/* 0x1010c */
	unsigned char	res46[0xf0];
	unsigned int	src_dmc;			/* 0x10200 */
	unsigned char	res47[0xfc];
	unsigned int	src_mask_dmc;			/* 0x10300 */
	unsigned char	res48[0xfc];
	unsigned int	mux_stat_dmc;			/* 0x10400 */
	unsigned char	res49[0xfc];
	unsigned int	div_dmc0;			/* 0x10500 */
	unsigned int	div_dmc1;			/* 0x10504 */
	unsigned char	res50[0xf8];
	unsigned int	div_stat_dmc0;			/* 0x10600 */
	unsigned int	div_stat_dmc1;			/* 0x10604 */
	unsigned char	res51[0xf8];
	unsigned int	gate_bus_dmc0;			/* 0x10700 */
	unsigned int	gate_bus_dmc1;			/* 0x10704 */
	unsigned char	res52[0xf8];
	unsigned int	gate_sclk_dmc;			/* 0x10800 */
	unsigned char	res53[0xfc];
	unsigned int	gate_ip_dmc0;			/* 0x10900 */
	unsigned int	gate_ip_dmc1;			/* 0x10904 */
	unsigned char	res54[0xf8];
	unsigned int	clkout_cmu_dmc;			/* 0x10a00 */
	unsigned int	clkout_cmu_dmc_div_stat;	/* 0x10a04 */
	unsigned char	res55[0x5f8];
	unsigned int	dcgidx_map0;			/* 0x11000 */
	unsigned int	dcgidx_map1;			/* 0x11004 */
	unsigned int	dcgidx_map2;			/* 0x11008 */
	unsigned char	res56[0x14];
	unsigned int	dcgperf_map0;			/* 0x11020 */
	unsigned int	dcgperf_map1;			/* 0x11024 */
	unsigned char	res57[0x18];
	unsigned int	dvcidx_map;			/* 0x11040 */
	unsigned char	res58[0x1c];
	unsigned int	freq_cpu;			/* 0x11060 */
	unsigned int	freq_dpm;			/* 0x11064 */
	unsigned char	res59[0x18];
	unsigned int	dvsemclk_en;			/* 0x11080 */
	unsigned int	maxperf;			/* 0x11084 */
	unsigned char	res60[0x8];
	unsigned int	dmc_freq_ctrl;			/* 0x11090 */
	unsigned int	dmc_pause_ctrl;			/* 0x11094 */
	unsigned int	ddrphy_lock_ctrl;		/* 0x11098 */
	unsigned int	c2c_state;			/* 0x1109c */
	unsigned char	res61[0x2f60];
	unsigned int	apll_lock;			/* 0x14000 */
	unsigned char	res62[0xfc];
	unsigned int	apll_con0;			/* 0x14100 */
	unsigned int	apll_con1;			/* 0x14104 */
	unsigned char	res63[0xf8];
	unsigned int	src_cpu;			/* 0x14200 */
	unsigned char	res64[0x1fc];
	unsigned int	mux_stat_cpu;			/* 0x14400 */
	unsigned char	res65[0xfc];
	unsigned int	div_cpu0;			/* 0x14500 */
	unsigned int	div_cpu1;			/* 0x14504 */
	unsigned char	res66[0xf8];
	unsigned int	div_stat_cpu0;			/* 0x14600 */
	unsigned int	div_stat_cpu1;			/* 0x14604 */
	unsigned char	res67[0x1f8];
	unsigned int	gate_sclk_cpu;			/* 0x14800 */
	unsigned char	res68[0xfc];
	unsigned int	gate_ip_cpu;			/* 0x14900 */
	unsigned char	res69[0xfc];
	unsigned int	clkout_cmu_cpu;			/* 0x14a00 */
	unsigned int	clkout_cmu_cpu_div_stat;	/* 0x14a04 */
	unsigned char	res70[0x5f8];
	unsigned int	armclk_stopctrl;		/* 0x15000 */
	unsigned int	atclk_stopctrl;			/* 0x15004 */
	unsigned int	arm_ema_ctrl;			/* 0x15008 */
	unsigned int	arm_ema_status;			/* 0x1500c */
	unsigned int	parityfail_status;		/* 0x15010 */
	unsigned int	parityfail_clear;		/* 0x15014 */
	unsigned char	res71[0x8];
	unsigned int	pwr_ctrl;			/* 0x15020 */
	unsigned int	pwr_ctrl2;			/* 0x15024 */
	unsigned char	res72[0xd8];
	unsigned int	apll_con0_l8;			/* 0x15100 */
	unsigned int	apll_con0_l7;
	unsigned int	apll_con0_l6;
	unsigned int	apll_con0_l5;
	unsigned int	apll_con0_l4;			/* 0x15110 */
	unsigned int	apll_con0_l3;
	unsigned int	apll_con0_l2;
	unsigned int	apll_con0_l1;
	unsigned int	iem_control;			/* 0x15120 */
	unsigned char	res73[0xdc];
	unsigned int	apll_con1_l8;			/* 0x15200 */
	unsigned int	apll_con1_l7;
	unsigned int	apll_con1_l6;
	unsigned int	apll_con1_l5;
	unsigned int	apll_con1_l4;			/* 0x15210 */
	unsigned int	apll_con1_l3;
	unsigned int	apll_con1_l2;
	unsigned int	apll_con1_l1;			/* 0x1521c */
	unsigned char	res74[0xe0];
	unsigned int	div_iem_l8;			/* 0x15300 */
	unsigned int	div_iem_l7;
	unsigned int	div_iem_l6;
	unsigned int	div_iem_l5;
	unsigned int	div_iem_l4;			/* 0x15310 */
	unsigned int	div_iem_l3;
	unsigned int	div_iem_l2;
	unsigned int	div_iem_l1;			/* 0x1531c */
	unsigned char	res75[0xe0];
	unsigned int	l2_status;			/* 0x15400 */
	unsigned char	res76[0xc];
	unsigned int	cpu_status;			/* 0x15410 */
	unsigned char	res77[0xc];
	unsigned int	ptm_status;			/* 0x15420 */
	unsigned char	res78[0x2edc];
	unsigned int	div_isp0;			/* 0x18300 */
	unsigned int	div_isp1;			/* 0x18304 */
	unsigned char	res79[0xf8];
	unsigned int	div_stat_isp0;			/* 0x18400 */
	unsigned int	div_stat_isp1;			/* 0x18404 */
	unsigned char	res80[0x2f8];
	unsigned int	gate_bus_isp0;			/* 0x18700 */
	unsigned int	gate_bus_isp1;			/* 0x18704 */
	unsigned int	gate_bus_isp2;			/* 0x18708 */
	unsigned int	gate_bus_isp3;			/* 0x1870c */
	unsigned char	res81[0xf0];
	unsigned int	gate_ip_isp0;			/* 0x18800 */
	unsigned int	gate_ip_isp1;			/* 0x18804 */
	unsigned char	res82[0xf8];
	unsigned int	gate_sclk_isp;			/* 0x18900 */
	unsigned char	res83[0xfc];
	unsigned int	clkout_cmu_isp;			/* 0x18a00 */
	unsigned int	clkout_cmu_isp_div_stat;	/* 0x18a04 */
	unsigned char	res84[0xf8];
	unsigned int	cmu_isp_spare0;			/* 0x18b00 */
	unsigned int	cmu_isp_spare1;			/* 0x18b04 */
	unsigned int	cmu_isp_spare2;			/* 0x18b08 */
	unsigned int	cmu_isp_spare3;			/* 0x18b0c */
#endif
};
#endif

#endif
