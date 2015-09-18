/*
 * Microchip PIC32 MUSB "glue layer"
 *
 * Copyright (C) 2015, Microchip Technology Inc.
 * Cristian Birsan <cristian.birsan@microchip.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 *
 * Based on the dsps "glue layer" code.
 */

#include <common.h>
#include "linux-compat.h"
#include "musb_core.h"
#include <asm/gpio.h>
#include <asm/arch/pic32.h>
#include <asm/arch/musb.h>

#define PIC32_TX_EP_MASK	0x0f		/* EP0 + 7 Tx EPs */
#define PIC32_RX_EP_MASK	0x0e		/* 7 Rx EPs */

#define	POLL_SECONDS		2

#define USBCRCON		0
#define USBCRCON_USBWKUPEN	0x00000001
#define USBCRCON_USBRIE		0x00000002
#define USBCRCON_USBIE		0x00000004
#define USBCRCON_PHYIDEN	0x00000080
#define USBCRCON_USBIDVAL	0x00000100
#define USBCRCON_USBIDOVEN	0x00000200
#define USBCRCON_USBWK		0x01000000
#define USBCRCON_USBRF		0x02000000
#define USBCRCON_USBIF		0x04000000

/**
 * pic32_musb_enable
 */
static void pic32_musb_enable(struct musb *musb)
{
	/* Add code if needed */
}

/**
 * dsps_musb_disable - disable HDRC and flush interrupts
 */
static void pic32_musb_disable(struct musb *musb)
{
	void __iomem *reg_base = musb->ctrl_base;

	u8 power;

	/* Reset the musb */
	power = musb_readb(reg_base, MUSB_POWER);
	power = power | MUSB_POWER_RESET;
	musb_writeb(reg_base, MUSB_POWER, power);

	mdelay(100);

	power = power & (~MUSB_POWER_RESET);
	musb_writeb(reg_base, MUSB_POWER, power);
}

static irqreturn_t pic32_interrupt(int irq, void *hci)
{
	struct musb  *musb = hci;
	void __iomem *reg_base = musb->ctrl_base;

	unsigned long flags;
	irqreturn_t ret = IRQ_NONE;
	u32 epintr, usbintr;

	spin_lock_irqsave(&musb->lock, flags);

	/* Get endpoint interrupts */
	musb->int_rx = musb_readw(reg_base, MUSB_INTRRX) & PIC32_RX_EP_MASK;
	musb->int_tx = musb_readw(reg_base, MUSB_INTRTX) & PIC32_TX_EP_MASK;

	/* Get usb core interrupts */
	musb->int_usb = musb_readb(reg_base, MUSB_INTRUSB);

	if (!musb->int_usb && !(musb->int_rx || musb->int_tx)) {
		dev_dbg(dev, "Got USB spurious interrupt !\n");
		goto eoi;
	}

	if (is_host_active(musb) && musb->int_usb & MUSB_INTR_BABBLE)
		dev_dbg(dev, "CAUTION: musb: Babble Interrupt Occurred\n");

	/* Drop spurious RX and TX if device is disconnected */
	if (musb->int_usb & MUSB_INTR_DISCONNECT) {
		musb->int_tx = 0;
		musb->int_rx = 0;
	}

	if (musb->int_tx || musb->int_rx || musb->int_usb)
		ret |= musb_interrupt(musb);

 eoi:
	spin_unlock_irqrestore(&musb->lock, flags);

	return ret;
}

static void __def_musb_init(void)
{
	int vbus_gpio = GPIO_PORT_PIN(PIC32_PORT_B, 5);

	/* Enable the VBUS switch: PB5 */
	gpio_request(vbus_gpio, "vbuson");
	gpio_direction_output(vbus_gpio, 1);

	/* Enable Pull Down resistor on USBID/RF3 to enable Host mode */
	writel(0x08, CNPDSET(PIC32_PORT_F)); /* CNPDF3 = 1 */
}

static void __def_musb_exit(void)
{
	int vbus_gpio = GPIO_PORT_PIN(PIC32_PORT_B, 5);

	/* Disable Pull Down resistor on USBID/RF3 to disable Host mode */
	writel(0x08, CNPDCLR(PIC32_PORT_F)); /* CNPDF3 = 0 */
	/* Reset the usb state machine */

	/* Disable the VBUS switch */
	gpio_set_value(vbus_gpio, 0); /* PB5 */
	gpio_free(vbus_gpio);
}

void board_musb_init(void) __attribute__((weak, alias("__def_musb_init")));
void board_musb_exit(void) __attribute__((weak, alias("__def_musb_exit")));

static int pic32_musb_set_mode(struct musb *musb, u8 mode)
{
	struct device *dev = musb->controller;
	void __iomem *glue_ctrl = (void __iomem *)PIC32_BASE_USB_CTRL;
	u32 crcon;

	switch (mode) {
	case MUSB_HOST:
		crcon = musb_readl(glue_ctrl, USBCRCON);
		musb_writel(glue_ctrl, USBCRCON,
			    (crcon | USBCRCON_USBIDOVEN) & ~USBCRCON_USBIDVAL);

		dev_dbg(dev, "MUSB Host mode enabled\n");

		break;
	case MUSB_PERIPHERAL:
		crcon = musb_readl(glue_ctrl, USBCRCON);
		musb_writel(glue_ctrl, USBCRCON,
			    crcon | USBCRCON_USBIDOVEN | USBCRCON_USBIDVAL);

		dev_dbg(dev, "MUSB Device mode enabled\n");

		break;
	case MUSB_OTG:
		/* TODO: Enable OTG mode */
		dev_dbg(dev, "MUSB OTG mode enabled\n");
		break;
	default:
		dev_err(glue->dev, "unsupported mode %d\n", mode);
		return -EINVAL;
	}

	return 0;
}
static int pic32_musb_init(struct musb *musb)
{
	u16 hwvers;
	u8 power;
	void __iomem *reg_base = musb->ctrl_base;

	board_musb_init();

	/* Returns zero if e.g. not clocked */
	hwvers = musb_read_hwvers(musb->mregs);
	if (!hwvers)
		return -ENODEV;

	/* Reset the musb */
	power = musb_readb(reg_base, MUSB_POWER);
	power = power | MUSB_POWER_RESET;
	musb_writeb(reg_base, MUSB_POWER, power);

	mdelay(100);

	power = power & (~MUSB_POWER_RESET);
	musb_writeb(reg_base, MUSB_POWER, power);

	/* Start the on-chip PHY and its PLL. */

	musb->isr = pic32_interrupt;
	musb_writel((void *)PIC32_BASE_USB_CTRL, USBCRCON,
		    USBCRCON_USBIF | USBCRCON_USBRF |
		    USBCRCON_USBWK | USBCRCON_USBIDOVEN |
		    USBCRCON_PHYIDEN | USBCRCON_USBIE |
		    USBCRCON_USBRIE | USBCRCON_USBWKUPEN);
	return 0;
}

static int pic32_musb_exit(struct musb *musb)
{
	struct device *dev = musb->controller;

	/* Shutdown the on-chip PHY and its PLL */
	board_musb_exit();

	return 0;
}

/* PIC32 supports only 32bit read operation */
void musb_read_fifo(struct musb_hw_ep *hw_ep, u16 len, u8 *dst)
{
	void __iomem *fifo = hw_ep->fifo;
	u32		val;
	int		i;

	/* Read for 32bit-aligned destination address */
	if (likely((0x03 & (unsigned long) dst) == 0) && len >= 4) {
		readsl(fifo, dst, len >> 2);
		dst += len & ~0x03;
		len &= 0x03;
	}
	/*
	 * Now read the remaining 1 to 3 byte or complete length if
	 * unaligned address.
	 */
	if (len > 4) {
		for (i = 0; i < (len >> 2); i++) {
			*(u32 *)dst = musb_readl(fifo, 0);
			dst += 4;
		}
		len &= 0x03;
	}
	if (len > 0) {
		val = musb_readl(fifo, 0);
		memcpy(dst, &val, len);
	}
}

const struct musb_platform_ops musb_pic32_ops = {
	.init		= pic32_musb_init,
	.exit		= pic32_musb_exit,
	.set_mode	= pic32_musb_set_mode,
	.enable		= pic32_musb_enable,
	.disable	= pic32_musb_disable,
};
