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
#include <asm/arch/clk.h>

/* Default is s5pc100 */
unsigned int s5p_cpu_id = 0xC100;
/* Default is EVT1 */
unsigned int s5p_cpu_rev = 1;

#ifdef CONFIG_ARCH_CPU_INIT
int arch_cpu_init(void)
{
	s5p_set_cpu_id();

	return 0;
}
#endif

u32 get_device_type(void)
{
	return s5p_cpu_id;
}

#ifdef CONFIG_DISPLAY_CPUINFO
int print_cpuinfo(void)
{
	char buf[32];
	char *cpu;
	unsigned int rev;

	switch (s5p_cpu_id) {
	case 0xE441:
		cpu = "EXYNOS4412";
		break;
	case 0x4322:
		cpu = "EXYNOS4212";
		break;
	case 0x4321:
		cpu = "EXYNOS4210";
		break;
	case 0xC110:
		cpu = "S5PC110";
		break;
	case 0xC100:
		cpu = "S5PC100";
		break;
	default:
		cpu = "(unknown)";
	}

	printf("CPU:   %s@%sMHz", cpu, strmhz(buf, get_arm_clk()));
	printf(", A:%sMHz, M:%sMHz\n",
		strmhz(buf, get_pll_clk(APLL)), strmhz(buf, get_pll_clk(MPLL)));

	rev = s5p_get_cpu_rev();
	/* only for over EVT1 */
	if (rev & 0xf0)
		printf("REV:   EVT %d.%d\n", (rev & 0xf0) >> 4, rev & 0xf);

	return 0;
}
#endif
