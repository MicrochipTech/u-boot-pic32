/*
 * Microchip PIC32 MUSB HCD (Host Controller Driver) for u-boot
 */

#include <common.h>
#include <asm/gpio.h>
#include <asm/arch-pic32/pic32.h>

#include "musb_core.h"

/* Timeout for PIC32 usb module */
#define PIC32_USB_OTG_TIMEOUT	0x3FFFFFF

/* No HUB support for now */
#define MUSB_NO_MULTIPOINT

/* MUSB platform configuration */
struct musb_config musb_cfg = {
	.regs		= (struct musb_regs *)PIC32_BASE_USB_CORE,
	.timeout	= PIC32_USB_OTG_TIMEOUT,
	.musb_speed	= 0,
};

/*
 * This function reads data from endpoint fifo(s) for MCHP USB
 * it uses 32 bit read operation
 *
 * ep           - endpoint number
 * length       - number of bytes to read from FIFO
 * fifo_data    - pointer to data buffer into which data is read
 */

void read_fifo(u8 ep, u32 length, void *fifo_data)
{
	u8  *data = (u8 *)fifo_data;
	u32 val;
	int i;

	/* select the endpoint index */
	writeb(ep, &musbr->index);

	if (length > 4) {
		for (i = 0; i < (length >> 2); i++) {
			val = readl(&musbr->fifox[ep]);
			memcpy(data, &val, 4);
			data += 4;
		}
		length %= 4;
	}
	if (length > 0) {
		val = readl(&musbr->fifox[ep]);
		memcpy(data, &val, length);
	}
}

/*
 * CPU and board-specific MUSB initializations.  Aliased function
 * signals caller to move on.
 */
static void __def_musb_init(void)
{
	/* Enable the VBUS switch: PB5 */
	gpio_request(37, "vbuson");
	gpio_direction_output(37, 1);

	/* Enable Pull Down resistor on USBID/RF3 to enable Host mode */
	writel(0x08, CNPDxSET(5)); /* CNPDF3 = 1 */

	/* 3 seconds delay required by errata */
	mdelay(3000);
}

static void musb_generic_stop(void)
{
	u16     temp;

	/* disable all interrupts */
	writeb(0, &musbr->intrusbe);
	writew(0, &musbr->intrtxe);
	writew(0, &musbr->intrrxe);

	/* off */
	writeb(0, &musbr->devctl);

	/*  flush pending interrupts */
	temp = readb(&musbr->intrusb);
	temp = readb(&musbr->intrtx);
	temp = readb(&musbr->intrrx);
	(void)temp;
}

static void __def_musb_deinit(void)
{
	/* Disable the VBUS switch */
	gpio_set_value(37, 0); /* PB5 */
	gpio_free(37);

	/* Disable Pull Down resistor on USBID/RF3 to disable Host mode */
	writel(0x08, CNPDxCLR(5)); /* CNPDF3 = 0 */
	mdelay(3000);

	musb_generic_stop();
}

void board_musb_init(void) __attribute__((weak, alias("__def_musb_init")));
void board_musb_deinit(void) __attribute__ ((weak, alias("__def_musb_deinit")));

/*
 * NOTE: Because of Microchip USB errata the USB module requires a start-up
 * delay. When enabling the USB PLL, add a three second delay before turning
 * on the USB module.
 */

int musb_platform_init(void)
{
	/* board specific initialization */
	board_musb_init();

	return 0;
}

/*
 * This function performs platform specific deinitialization for usb.
*/
void musb_platform_deinit(void)
{
	board_musb_deinit();
}
