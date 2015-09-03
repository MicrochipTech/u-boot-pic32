/*
 * Copyright (C) 2015 Purna Chandra Mandal <purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch-pic32/pic32.h>
#include <asm/arch-pic32/ap.h>

/* Fixed clk rate */
#define SYS_FRC_CLK_HZ	8000000

/* PLL */
#define ICLK_MASK	0x00000080
#define PLLIDIV_MASK	0x00000007
#define PLLODIV_MASK	0x00000007
#define CUROSC_MASK	0x00000007
#define PLLMUL_MASK	0x0000007F
#define FRCDIV_MASK	0x00000007

/* PBCLK */
#define PBDIV_MASK	0x00000007

/* SYSCLK MUX */
#define SCLK_SRC_FRC1	0
#define SCLK_SRC_SPLL	1
#define SCLK_SRC_POSC	2
#define SCLK_SRC_FRC2	7

/* Reference Oscillator Control Reg fields */
#define REFO_SEL_MASK	0x0f
#define REFO_SEL_SHIFT	0
#define REFO_ACTIVE	0x0100
#define REFO_DIVSW_EN	0x0200
#define REFO_OE		0x1000
#define REFO_ON		0x8000
#define REFO_DIV_SHIFT	16
#define REFO_DIV_MASK	0x7fff

/* Reference Oscillator Trim Register Fields */
#define REFO_TRIM_REG	0x10 /* Register offset w.r.t. REFO_CON_REG */
#define REFO_TRIM_MASK	0x1ff
#define REFO_TRIM_SHIFT	23
#define REFO_TRIM_MAX	511

#define ROCLK_SRC_SCLK		0x0
#define ROCLK_SRC_SPLL		0x7
#define ROCLK_SRC_ROCLKI	0x8

/* Memory PLL */
#define MPLL_IDIV		0x03
#define MPLL_MULT		0x32
#define MPLL_ODIV1		0x02
#define MPLL_ODIV2		0x01
#define MPLL_VREG_RDY		0x00800000
#define MPLL_RDY		0x80000000
#define MPLL_IDIV_SHIFT		0
#define MPLL_MULT_SHIFT		8
#define MPLL_ODIV1_SHIFT	24
#define MPLL_ODIV2_SHIFT	27

ulong pic32_get_pllclk(void)
{
	ulong plliclk, v;
	u32 iclk, idiv, odiv, mult;

	v = readl(SPLLCON);
	iclk = (v & ICLK_MASK);
	idiv = ((v >> 8) & PLLIDIV_MASK) + 1;
	odiv = ((v >> 24) & PLLODIV_MASK);
	mult = ((v >> 16) & PLLMUL_MASK) + 1;

	plliclk = iclk ? SYS_FRC_CLK_HZ : CONFIG_PIC32_POSC_FREQ;

	if (odiv < 2)
		odiv = 2;
	else if (odiv < 5)
		odiv = (1 << odiv);
	else
		odiv = 32;

	return ((plliclk / idiv) * mult) / odiv;
}

static ulong pic32_get_sysclk(void)
{
	ulong hz;
	ulong div, frcdiv;
	ulong v  = readl(OSCCON);
	ulong curr_osc;

	/* get clk source */
	v = readl(OSCCON);
	curr_osc = (v >> 12) & CUROSC_MASK;

	switch (curr_osc) {
	case SCLK_SRC_FRC1:
	case SCLK_SRC_FRC2:
		frcdiv = ((v >> 24) & FRCDIV_MASK);
		div = ((1 << frcdiv) + 1) + (128 * (frcdiv == 7));
		hz = SYS_FRC_CLK_HZ / div;
		break;

	case SCLK_SRC_SPLL:
		hz = pic32_get_pllclk();
		break;

	case SCLK_SRC_POSC:
		hz = CONFIG_PIC32_POSC_FREQ;
		break;

	default:
		hz = 0;
		break;
	}

	return hz;
}

ulong pic32_get_pbclk(int bus)
{
	ulong div, clk_freq;
	void __iomem *reg;

	clk_freq = pic32_get_sysclk();

	reg = (void __iomem *)PB1DIV + ((bus - 1) * 0x10);
	div = (readl(reg) & PBDIV_MASK) + 1;

	return clk_freq / div;
}

ulong pic32_get_cpuclk(void)
{
	return pic32_get_pbclk(7);
}

ulong pic32_set_refclk(int bus, int parent_rate, int rate, int parent_id)
{
	void __iomem *reg;
	u32 div, trim, v;

	/* calculate dividers */
	if (parent_rate <= rate) {
		div = 0;
		trim = 0;
	} else {
		div = parent_rate / (rate << 1);
		trim = 0;
	}

	reg = (void __iomem *)(REFO1CON + (bus - 1) * 0x20);

	/* wait till previous src change is active */
	for (;;) {
		v = readl(reg);
		if ((v & (REFO_DIVSW_EN|REFO_ACTIVE)) == 0)
			break;
	}

	/* parent_id */
	v &= ~(REFO_SEL_MASK << REFO_SEL_SHIFT);
	v |= (parent_id << REFO_SEL_SHIFT);

	/* apply rodiv */
	v &= ~(REFO_DIV_MASK << REFO_DIV_SHIFT);
	v |= (div << REFO_DIV_SHIFT);
	writel(v, reg);

	/* apply trim */
	writel(trim, reg + REFO_TRIM_REG);

	/* enable clk */
	writel(REFO_ON|REFO_OE, reg + _SET_OFFSET);

	/* switch divider */
	writel(REFO_DIVSW_EN, reg + _SET_OFFSET);

	for (;;) {
		if (!(readl(reg) & REFO_DIVSW_EN))
			break;
	}

	return 0;
}

ulong pic32_get_refclk(int bus)
{
	void __iomem *reg;
	u32 rodiv, rotrim, rosel, v, parent_rate;

	reg  = (void __iomem *)(REFO1CON + (bus - 1) * 0x20);
	v = readl(reg);

	/* get rosel */
	rosel = (v >> REFO_SEL_SHIFT) & REFO_SEL_MASK;
	/* get div */
	rodiv = (v >> REFO_DIV_SHIFT) & REFO_DIV_MASK;
	/* get trim */
	v = readl(reg + REFO_TRIM_REG);
	rotrim = (v >> REFO_TRIM_SHIFT) & REFO_TRIM_MASK;

	if (rotrim)
		printf("refo%dclk: rotrim non zero\n", bus);

	/* calc rate */
	switch (rosel) {
	default:
	case ROCLK_SRC_SCLK:
		parent_rate = pic32_get_cpuclk();
		break;
	case ROCLK_SRC_SPLL:
		parent_rate = pic32_get_pllclk();
		break;
	}
	v = parent_rate / (rodiv << 1);
	return v;
}

void mpll_init(void)
{
	u32 v, mask;

	/* initialize */
	v = (MPLL_IDIV << MPLL_IDIV_SHIFT)|(MPLL_MULT << MPLL_MULT_SHIFT)|
	    (MPLL_ODIV1 << MPLL_ODIV1_SHIFT)|(MPLL_ODIV2 << MPLL_ODIV2_SHIFT);

	writel(v, (void __iomem *)CFGMPLL);

	/* Wait for ready */
	mask = MPLL_RDY | MPLL_VREG_RDY;
	for (;;) {
		v = readl((void __iomem *)CFGMPLL);
		if ((v & mask) == mask)
			break;
	}
}

void clk_init(void)
{
	ulong pll_hz = pic32_get_pllclk();

	debug("PLL Speed: %lu MHz\n", pll_hz / 1000000);
#ifdef CONFIG_SYS_PIC32_REFO1_HZ
	pic32_set_refclk(1, pll_hz, CONFIG_SYS_PIC32_REFO1_HZ, ROCLK_SRC_SPLL);
	debug("refosc1 Speed: %lu MHz\n", pic32_get_refclk(1) / 1000000);
#endif
#ifdef CONFIG_SYS_PIC32_REFO2_HZ
	pic32_set_refclk(2, pll_hz, CONFIG_SYS_PIC32_REFO2_HZ, ROCLK_SRC_SPLL);
	debug("refosc2 Speed: %lu MHz\n", pic32_get_refclk(2) / 1000000);
#endif

#ifdef CONFIG_SYS_PIC32_REFO3_HZ
	pic32_set_refclk(3, pll_hz, CONFIG_SYS_PIC32_REFO3_HZ, ROCLK_SRC_SPLL);
	debug("refosc3 Speed: %lu MHz\n", pic32_get_refclk(3) / 1000000);
#endif

#ifdef CONFIG_SYS_PIC32_REFO4_HZ
	pic32_set_refclk(4, pll_hz, CONFIG_SYS_PIC32_REFO4_HZ, ROCLK_SRC_SPLL);
	debug("refosc4 Speed: %lu MHz\n", pic32_get_refclk(4) / 1000000);
#endif
#ifdef CONFIG_TARGET_PIC32MZDASK
	mpll_init();
#endif
}

#ifdef CONFIG_CMD_CLK
int soc_clk_dump(void)
{
	int i;
	ulong pll_hz = pic32_get_pllclk();

	printf("PLL Speed: %lu MHz\n", pll_hz / 1000000);

	printf("CPU Clock Speed: %lu MHz\n", pic32_get_cpuclk() / 1000000);
	for (i = 1; i < 7; i++)
		printf("periph%dclk speed: %lu MHz\n",
		       i, pic32_get_pbclk(i) / 1000000);

#ifdef CONFIG_SYS_PIC32_REFO1_HZ
	i = 1;
	printf("refosc%dclk speed: %lu MHz\n",
	       i, pic32_get_refclk(i) / 1000000);
#endif
#ifdef CONFIG_SYS_PIC32_REFO2_HZ
	i = 2;
	printf("refosc%dclk speed: %lu MHz\n",
	       i, pic32_get_refclk(i) / 1000000);
#endif
#ifdef CONFIG_SYS_PIC32_REFO3_HZ
	i = 3;
	printf("refosc%dclk speed: %lu MHz\n",
	       i, pic32_get_refclk(i) / 1000000);
#endif
#ifdef CONFIG_SYS_PIC32_REFO4_HZ
	i = 4;
	printf("refosc%dclk speed: %lu MHz\n",
	       i, pic32_get_refclk(i) / 1000000);
#endif
#ifdef CONFIG_SYS_PIC32_REFO5_HZ
	i = 5;
	printf("refosc%dclk speed: %lu MHz\n",
	       i, pic32_get_refclk(i) / 1000000);
#endif
	return 0;
}
#endif
