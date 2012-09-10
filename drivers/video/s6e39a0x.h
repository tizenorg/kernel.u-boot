/* linux/drivers/video/samsung/s6e39a0x.h
 *
 * MIPI-DSI based s6e39a0x AMOLED LCD Panel definitions. 
 *
 * Copyright (c) 2010 Samsung Electronics
 * Donghwa Lee <dh09.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _S6E39A0X_H
#define _S6E39A0X_H

extern void s6e39a0x_init(void);
extern void s6e39a0x_set_link(struct mipi_ddi_platform_data *pd);
extern int s6e39a0x_panel_init(void);
extern int s6e39a0x_display_on(void);

#endif //_S6E39A0X_H

