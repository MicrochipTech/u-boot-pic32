/*
 * Copyright (C) 2014
 * Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#include <common.h>
#include <linux/compiler.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/musb.h>
#include <asm/io.h>
#include <asm/mipsregs.h>
#include <asm/arch/pic32.h>
#include <asm/arch/ap.h>
#include <asm/arch/musb.h>

DECLARE_GLOBAL_DATA_PTR;

/* initialize prefetch module based on SYSCLK */
int init_prefetch(void)
{
	u8 dyn_ecc;
	int nr_waits;
	ulong sysclk;

	sysclk = pic32_get_cpuclk();

	dyn_ecc = (readl(CFGCON) >> 4) & 0x3; /* ECCCON */
	dyn_ecc = (dyn_ecc < 2) ? 1 : 0; /* dynamic ECC enabled */
	if (sysclk <= (dyn_ecc ? 66000000 : 83000000))
		nr_waits = 0;
	else if (sysclk <= (dyn_ecc ? 133000000 : 166000000))
		nr_waits = 1;
	else
		nr_waits = 2;

	writel(nr_waits, PRECON);
	writel(0x30, PRECONSET); /* Enable prefetch for all */

	return 0;
}

#ifdef CONFIG_DISPLAY_BOARDINFO
char *get_boardinfo(void)
{
	u32 proc_id;
	char *str;

	proc_id = read_c0_prid();
	switch (proc_id) {
	case 0x00018000:
		str = "4Kc";
		break;
	case 0x00018400:
		str = "4KEcR1";
		break;
	case 0x00019000:
		str = "4KEc";
		break;
	case 0x00019e28:
		str = "PIC32MZ[DA]";
		break;
	case 0x0001a720:
		str = "PIC32MZ[EF]";
		break;
	default:
		str = "UNKNOWN";
	}

	return str;
}
#endif

/* PIC32 has one MUSB controller which can be host or gadget */
#if (defined(CONFIG_MUSB_GADGET) || defined(CONFIG_MUSB_HOST)) && \
	(defined(CONFIG_PIC32_USB0))

/* Microchip FIFO config 0 - fits in 8KB */
static struct musb_fifo_cfg microchip_musb_fifo_cfg0[] = {
	{ .hw_ep_num = 1, .style = FIFO_TX, .maxpacket = 512, },
	{ .hw_ep_num = 1, .style = FIFO_RX, .maxpacket = 512, },
	{ .hw_ep_num = 2, .style = FIFO_TX, .maxpacket = 512, },
	{ .hw_ep_num = 2, .style = FIFO_RX, .maxpacket = 512, },
	{ .hw_ep_num = 3, .style = FIFO_TX, .maxpacket = 512, },
	{ .hw_ep_num = 3, .style = FIFO_RX, .maxpacket = 512, },
	{ .hw_ep_num = 4, .style = FIFO_TX, .maxpacket = 512, },
	{ .hw_ep_num = 4, .style = FIFO_RX, .maxpacket = 512, },
	{ .hw_ep_num = 5, .style = FIFO_TX, .maxpacket = 512, },
	{ .hw_ep_num = 5, .style = FIFO_RX, .maxpacket = 512, },
	{ .hw_ep_num = 6, .style = FIFO_TX, .maxpacket = 512, },
	{ .hw_ep_num = 6, .style = FIFO_RX, .maxpacket = 512, },
	{ .hw_ep_num = 7, .style = FIFO_TX, .maxpacket = 512, },
	{ .hw_ep_num = 7, .style = FIFO_RX, .maxpacket = 512, },
};

static struct musb_hdrc_config musb_config = {
	.fifo_cfg	= microchip_musb_fifo_cfg0,
	.fifo_cfg_size	= ARRAY_SIZE(microchip_musb_fifo_cfg0),
	.multipoint     = 1,
	.dyn_fifo       = 1,
	.num_eps        = 8,
	.ram_bits       = 11,
};

#ifdef CONFIG_PIC32_USB0
static void pic32_otg0_set_phy_power(u8 on)
{
	/* Add phy code when needed */
}

struct pic32_musb_board_data otg0_board_data = {
	.set_phy_power = pic32_otg0_set_phy_power,
};

static struct musb_hdrc_platform_data otg0_plat = {
	.mode           = CONFIG_PIC32_USB0_MODE,
	.config         = &musb_config,
	.power          = 50,
	.platform_ops	= &musb_pic32_ops,
	.board_data	= &otg0_board_data,
};
#endif
#endif

int arch_misc_init(void)
{
	bd_t *bd = gd->bd;

	/*
	 * Save local variables to board info struct
	 */
	bd->bi_memstart = pic32_virt_to_phys((void *)CONFIG_SYS_SDRAM_BASE);
	bd->bi_memsize = gd->ram_size;

#ifdef CONFIG_USB_MUSB_PIC32
	musb_register(&otg0_plat, &otg0_board_data,
		      (void *)PIC32_BASE_USB_CORE);
#endif
	return 0;
}
