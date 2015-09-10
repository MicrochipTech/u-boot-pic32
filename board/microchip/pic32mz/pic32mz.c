/*
 * (c) 2014 Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <netdev.h>
#include <miiphy.h>
#include <spi.h>
#include <linux/sizes.h>
#include <asm/gpio.h>
#include <asm/mipsregs.h>

#include <asm/arch-pic32/pic32.h>
#include <asm/arch-pic32/ap.h>

/* initializes board specific registers before relocation */
int board_early_init_f(void)
{
	clk_init();
	init_prefetch();

	/* PPS for U2 RX/TX */
	writel(0x0001, ANSELCLR(PIC32_PORT_D)); /* RPD0 */
	writel(0x0001, ANSELCLR(PIC32_PORT_B)); /* RPB0 */

	writel(0x02, RPD0R); /* U2TX -> RPD0 */
	writel(0x05, U2RXR); /* U2RX <- RPB0 */
	return 0;
}

/* initialize the DDR and PHY */
phys_size_t initdram(int board_type)
{
	int ntlb = 0;

#if defined(CONFIG_SYS_PIC32_PLANB) || defined(CONFIG_SYS_PIC32_PLANC) || defined(CONFIG_SYS_PIC32_PLAND)
	/* setup SRAM - PLAN-B/C daughter board */
	setup_ebi_sram();
	printf("EBI: SRAM configured\n");

	write_c0_wired(0);

	/* TLB map external memory to KSEG2 */
	write_one_tlb(0,        /* index */
		      PM_4M,	/* Pagemask, 4-MB pages */
		      KSEG2,	/* Hi */
		      ENTRYLO_CAC(0x20000000),		/* Lo0 */
		      ENTRYLO_CAC(0x20000000 + SZ_4M));	/* Lo1 */
	ntlb++;

	write_one_tlb(1, PM_4M, KSEG3, ENTRYLO_UNC(0x20000000),
		      ENTRYLO_UNC(0x20000000 + SZ_4M));
	ntlb++;

#if defined(CONFIG_SYS_PIC32_PLANC) || defined(CONFIG_SYS_PIC32_PLAND)
	write_one_tlb(2, PM_4M, KSEG2 + SZ_8M, ENTRYLO_CAC(0x20000000 + SZ_8M),
		      ENTRYLO_CAC(0x20000000 + SZ_8M + SZ_4M));
	ntlb++;

	write_one_tlb(3, PM_4M, KSEG3 + SZ_8M, ENTRYLO_UNC(0x20000000 + SZ_8M),
		      ENTRYLO_UNC(0x20000000 + SZ_8M + SZ_4M));
	ntlb++;
#endif
	debug("EBI: SRAM mapped to 0x%x\n", KSEG2);
#endif

#if defined(CONFIG_PIC32_SQI_XIP)
	/* SQI XIP: external memory */
	write_one_tlb(ntlb, PM_4M, CONFIG_SYS_PIC32_SQI_XIP_BASE,
		      ENTRYLO_UNC(0x30000000), ENTRYLO_UNC(0x30000000 + SZ_4M));
	ntlb++;
#endif
	write_c0_wired(ntlb);

	/*run_ebi_sram_test(SZ_16M);*/

	/* TODO - add support of external DDR when available. */
	return CONFIG_SYS_MEM_SIZE;
}

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

/* if networking is enabled, initialize the ethernet subsystem */
#ifdef CONFIG_CMD_NET
#if defined(CONFIG_SYS_PIC32_PLAND)
void board_netphy_reset(void)
{
	static int phy_rst_init;
	int rst_gpio = GPIO_PORT_PIN(PIC32_PORT_H, 11);

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
#endif

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
	 * Reg    Bit  I/O    Dig/Ana
	 * ECRSDV RH13 Input  Digital
	 * ERXD0  RH8  Input  Digital
	 * ERXD1  RH5  Input  Digital
	 * ERXERR RH4  Input  Digital
	 */
	writel(0x2130, ANSELCLR(PIC32_PORT_H));  /* set to digital mode */
	writel(0x2130, TRISSET(PIC32_PORT_H));   /* set to input mode */

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

#if defined(CONFIG_PIC32_SPI)
/* GPIO controller driven SPI CS */
void spi_cs_deactivate(struct spi_slave *slave)
{
#if defined(CONFIG_SYS_PIC32_PLAND)
	gpio_set_value(GPIO_PORT_PIN(PIC32_PORT_B, 14), 1);
#else
	gpio_set_value(GPIO_PORT_PIN(PIC32_PORT_D, 4), 1);
#endif
}

void spi_cs_activate(struct spi_slave *slave)
{
#if defined(CONFIG_SYS_PIC32_PLAND)
	gpio_set_value(GPIO_PORT_PIN(PIC32_PORT_B, 14), 0);
#else
	gpio_set_value(GPIO_PORT_PIN(PIC32_PORT_D, 4), 0);
#endif
}

void spi_init(void)
{
#if defined(CONFIG_SYS_PIC32_PLANB) || defined(CONFIG_SYS_PIC32_PLANC)
	/* SCK1 -> J1-118 */

	/* SDI1 -> J1-166(RPG8) -> MEB J4 */
	writel(0x0001, SDI1R);
	writel(0x0100, ANSELCLR(PIC32_PORT_G));  /* digital */
	writel(0x0100, TRISSET(PIC32_PORT_G));   /* input */

	/* SDO1 -> J1-124 (RPA14) -> MEB J4 */
	writel(0x5, RPA14R);
	writel(0x4000, ANSELCLR(PIC32_PORT_A));  /* digital */
	writel(0x4000, TRISCLR(PIC32_PORT_A));   /* output */

	/* GPIO controlled spi-cs. */
	gpio_request(GPIO_PORT_PIN(PIC32_PORT_D, 4), "spi1-cs0");
	gpio_direction_output(GPIO_PORT_PIN(PIC32_PORT_D, 4), 1);

#elif defined(CONFIG_SYS_PIC32_PLAND)
	/*
	 * RPD7 : SDI2
	 * RPG8 : SDO2
	 * RPB14: CS/SS
	 * RPG6 : SCK2
	 */
	/* SDI2R <= RPD7(0xe) */
	writel(0x0e, SDI2R);
	writel(0x80, ANSELCLR(PIC32_PORT_D));  /* digital */
	writel(0x80, TRISSET(PIC32_PORT_D));   /* input */

	/* RPG8R <= SDO2(0x6) */
	writel(0x6, RPG8R);
	writel(0x0100, ANSELCLR(PIC32_PORT_G));  /* digital */
	writel(0x0100, TRISCLR(PIC32_PORT_G));   /* output */

	/* SS(GPIO30) */
	gpio_request(GPIO_PORT_PIN(PIC32_PORT_B, 14), "spi2_cs0");
	gpio_direction_output(GPIO_PORT_PIN(PIC32_PORT_B, 14), 1);
#endif
}
#endif

