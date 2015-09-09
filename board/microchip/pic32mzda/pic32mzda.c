/*
 * Microchip PIC32MZ[DA]Starter Kit board
 *
 * Copyright (C) 2015, Microchip Technology Inc.
 * Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#include <common.h>
#include <command.h>
#include <netdev.h>
#include <miiphy.h>
#include <linux/sizes.h>
#include <asm/mipsregs.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch-pic32/pic32.h>
#include <asm/arch-pic32/ap.h>

#include "ddr.h"

/* initializes board specific registers before relocation */
int board_early_init_f(void)
{
	/* flash prefetch */
	init_prefetch();

	/* clock */
	clk_init();

	/* PPS for U2 RX/TX */
	writel(0x02, RPG9R); /* G9 */
	writel(0x05, U2RXR); /* B0 */
	writel(0x01, ANSELCLR(PIC32_PORT_B)); /* digital mode */

#if defined(CONFIG_PIC32_SQI_XIP)
	/* SQI XIP: external memory */
	write_one_tlb(0, PM_4M, CONFIG_SYS_PIC32_SQI_XIP_BASE,
		      ENTRYLO_CAC(CONFIG_SYS_PIC32_SQI_XIP_PHYS),
		      ENTRYLO_CAC(CONFIG_SYS_PIC32_SQI_XIP_PHYS + SZ_4M));
	write_c0_wired(1);
#endif

	return 0;
}

/* initialize the DDR and PHY */
phys_size_t initdram(int board_type)
{
	ddr_phy_init();
	ddr_init();
	return CONFIG_SYS_MEM_SIZE;
}

#ifdef CONFIG_SYS_DRAM_TEST
int testdram(void)
{
	run_memory_test(SZ_4M, (void *)CONFIG_SYS_MEMTEST_START);
	return 0;
}
#endif

#ifdef CONFIG_DISPLAY_BOARDINFO
int checkboard(void)
{
	printf("Board: %s\n", get_boardinfo());
	printf("CPU Speed: %lu MHz\n", pic32_get_cpuclk() / 1000000);
	return 0;
}
#endif

int misc_init_r(void)
{
	set_io_port_base(0);
	return 0;
}

#ifdef CONFIG_CMD_NET
void board_netphy_reset(void)
{
	static int phy_rst_init;
	int rst_gpio = GPIO_PORT_PIN(PIC32_PORT_J, 15);

	if (!phy_rst_init) {
		gpio_request(rst_gpio, "phy_rst");
		gpio_direction_output(rst_gpio, 1);
		phy_rst_init = 1;
	}

	/* gpio reset */
	gpio_set_value(rst_gpio, 0);
	udelay(300);
	gpio_direction_output(rst_gpio, 1);
	udelay(300);
}

/* config PIC32MZ io pins to send packets to/from the Phy module */
static void pic32_config_io_pins(void)
{
	/*
	 * PORT D pin configuration settings
	 *
	 * Reg   Bit  I/O    Dig/Ana
	 * EMDC  RD11 Output Digital
	 * ETXEN RD6  Output Digital
	 *
	 */
	writel(0x0840, ANSELCLR(PIC32_PORT_D));  /* set to digital mode */
	writel(0x0840, TRISCLR(PIC32_PORT_D));   /* set to output mode  */

	/*
	 * PORT H pin configuration settings
	 *
	 * Reg    Bit  I/O    Dig/Ana   PullUp/Down
	 * ECRSDV RH13 Input  Digital
	 * ERXD0  RH8  Input  Digital   Down
	 * ERXD1  RH5  Input  Digital   Down
	 */
	writel(0x2120, ANSELCLR(PIC32_PORT_H));  /* set to digital mode */
	writel(0x2120, TRISSET(PIC32_PORT_H));   /* set to input mode */

	/*
	 * PORT J pin configuration settings
	 *
	 * Reg     Bit  I/O    Dig/Ana
	 * EREFCLK RJ11 Input  Digital
	 * ETXD1   RJ9  Output Digital
	 * ETXD0   RJ8  Output Digital
	 * EMDIO   RJ1  Input  Digital
	 *
	 */
	writel(0x0b02, ANSELCLR(PIC32_PORT_J)); /* set to digital mode */
	writel(0x0300, TRISCLR(PIC32_PORT_J));  /* set pins to output mode  */
	writel(0x0802, TRISSET(PIC32_PORT_J));  /* set pins to input mode  */

	/*
	 * PORT F pin configuration settings
	 * Reg    Bit  I/O    Dig/Ana
	 * ERXERR RF3  Input  Digital
	 */
	writel(0x10, ANSELCLR(PIC32_PORT_F));  /* set to digital mode */
	writel(0x10, TRISSET(PIC32_PORT_F));   /* set to input mode */

	return;
}

int board_eth_init(bd_t *bis)
{
	int ret;

	/* config PIC32MZ io pins to send packets to/from the Phy module */
	pic32_config_io_pins();

	ret = pic32eth_initialize(bis, ETHCON1_BASE, CONFIG_SYS_PHY_ADDR,
				  PHY_INTERFACE_MODE_RMII);
	return ret;
}
#endif

#ifdef CONFIG_PIC32_SDHCI
int board_mmc_init(bd_t *bis)
{
	return pic32_sdhci_init(SDHC_BASE, SDHC_MAX_CLK, SDHC_MIN_CLK);
}
#endif
