/*
 * Board data structure for musb gadget on PIC32
 *
 * Copyright (C) 2015, Cristian Birsan <cristian.birsan@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ASM_MIPS_PIC32_MUSB_H
#define __ASM_MIPS_PIC32_MUSB_H

extern const struct musb_platform_ops musb_pic32_ops;

struct pic32_musb_board_data {
	u8 interface_type;
	void (*set_phy_power)(u8 on);
};

enum musb_interface    {MUSB_INTERFACE_ULPI, MUSB_INTERFACE_UTMI};
#endif /* __ASM_MIPS_PIC32_MUSB_H */
