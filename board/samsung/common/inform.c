#include <common.h>
#include <asm/arch/power.h>

#define	CONFIG_NORMAL_INFORM	0x12345678

void board_inform_clear(void)
{
	struct exynos4_power *power =
		(struct exynos4_power *)samsung_get_base_power();

	/* clear INFORM3 - reset status */
	writel(0, &power->inform3);

	/* set INFORM2 for normal boot */
	writel(CONFIG_NORMAL_INFORM, &power->inform2);
}

