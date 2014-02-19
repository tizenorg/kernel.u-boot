/*
 * Simple xorshift PRNG
 *   see http://www.jstatsoft.org/v08/i14/paper
 *
 * Copyright (c) 2012 Michael Walle
 * Michael Walle <michael@walle.cc>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>

static unsigned int y = 1U;

unsigned int rand_r(unsigned int *seedp)
{
	*seedp ^= (*seedp << 13);
	*seedp ^= (*seedp >> 17);
	*seedp ^= (*seedp << 5);

	return *seedp;
}

unsigned int rand(void)
{
#ifdef CONFIG_RAND_HW_ACCEL
	return hw_rand();
#else
	return rand_r(&y);
#endif
}

void srand(unsigned int seed)
{
	y = seed;
}
