/*
 * (c) 2015 Paul Thacker <paul.thacker@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#include <asm/io.h>
#include <stdint.h>
#include <config.h>
#include <asm/mipsregs.h>
#include <asm/arch/pic32.h>

#include "ddr.h"

/* macros */
#define max(a, b)		(((a) > (b)) ? (a) : (b))
#define div_round_up(x, y)	(((x) + (y) - 1) / (y))
#define hc_clk_dly(dly)		(max((div_round_up((dly), CLK_PERIOD)), 2) - 2)

/* Host Commands */
#define IDLE_NOP		0x00FFFFFF
#define PRECH_ALL_CMD		0x00FFF401
#define REF_CMD			0x00FFF801
#define LOAD_MODE_CMD		0x00FFF001
#define CKE_LOW			0x00FFEFFE

#define NUM_HOST_CMDS		12

/* DDR address decoding */
#define COL_HI_RSHFT		0
#define COL_HI_MASK		0
#define COL_LO_MASK		((1 << COL_BITS) - 1)

#define BA_RSHFT		COL_BITS
#define BANK_ADDR_MASK		((1 << BA_BITS) - 1)

#define ROW_ADDR_RSHIFT		(BA_RSHFT + BA_BITS)
#define ROW_ADDR_MASK		((1 << ROW_BITS) - 1)

/* #define CS_ADDR_RSHIFT          (ROW_ADDR_RSHIFT + ROW_BITS) */
#define CS_ADDR_RSHIFT		0
#define CS_ADDR_MASK		0

/* MPLL freq is 400MHz */
#define CLK_PERIOD              2500    /* 2500 psec */
#define CTRL_CLK_PERIOD         (CLK_PERIOD * 2)

/* Arbiter */
#define NUM_AGENTS              5
#define MIN_LIM_WIDTH           5
#define RQST_PERIOD_WIDTH       8
#define MIN_CMDACPT_WIDTH       8

#define EN_AUTO_PRECH           0
#define SB_PRI                  1
#define BIG_ENDIAN              0
#define HALF_RATE_MODE          1

/*****************************************************************************/
/* DDR definitions */
/*****************************************************************************/

/* DDR Address Mapping: CS, ROW, BA, COL */
#define COL_BITS	10
#define ROW_BITS	13
#define BA_BITS		3
#define CS_BITS		1

/* DDR constants */
#define BL		2    /* Burst length in cycles */

/* default CAS latency for all speed grades */
#define RL		5

/* default write latency for all speed grades = CL-1 */
#define WL		4

#define MAX_BURST	3

/* NOTE: DDR data from Micron MT47H64M16HR-3 data sheet */
#define tRFC_MIN	127500	/* psec */
#define tWR		15000	/* psec */
#define tRP		12500	/* psec */
#define tRCD		12500	/* psec */
#define tRRD		7500	/* psec */

/* tRRD_TCK is a minimum of 2 clk periods, regardless of clk freq */
#define tRRD_TCK	2
#define tWTR		7500	/* psec */

/* tWTR_TCK is a minimum of 2 clk periods, regardless of clk freq */
#define tWTR_TCK	2
#define tRTP		7500	/* psec */
#define tRTP_TCK	(BL / 2)
#define tXP_TCK		2	/* clocks */
#define tCKE_TCK	3	/* clocks */
#define tXSNR		(tRFC_MIN + 10000) /* psec */
#define tDLLK		200     /* clocks */
#define tRAS_MIN	45000   /* psec */
#define tRC		57500   /* psec */
#define tFAW		35000   /* psec */
#define tMRD		2       /* clocks */
#define tRFI		7800000 /* psec */

/* Refresh Config */
#define MAX_PEND_REF            7

/* Power Config */
#define PRECH_PWR_DN_ONLY       0
#define SELF_REF_DLY            17
#define PWR_DN_DLY              8
#define EN_AUTO_SELF_REF        0
#define EN_AUTO_PWR_DN          0
#define ERR_CORR_EN             0
#define ECC_EN                  0

/* PHY PAD CONTROL */
#define ODT_SEL                 1
#define ODT_EN                  1
#define DRIVE_SEL               0
#define ODT_PD                  2
#define ODT_PU                  3
#define EXTRA_OEN_CLK           0
#define NOEXT_DLL               1
#define HALF_RATE               1
#define DLR_DFI_WRCMD		1
#define DRVSTR_PFET             0xE
#define DRVSTR_NFET             0xE
#define RCVR_EN                 1
#define PREAMBLE_DLY            2

/* PHY DLL RECALIBRATE */
#define RECALIB_CNT             0x10
#define DELAY_START_VAL		3

/* SCL CONFIG */
#define SCL_BURST8              1
#define SCL_DDR2_CONNECTED	1
#define SCL_ODTCSWW		1
#define SCL_CSEN		1
#define SCL_CAPCLKDLY		3
#define SCL_DDRCLKDLY		4

/* SCL START */
#define SCL_START		0x10000000
#define SCL_EN			0x04000000
#define SCL_LUBPASS		3

void ddr_pmd_unlock(void)
{
	writel(0, PMD7);
}

void ddr_phy_init(void)
{
	uint32_t phy_pad_ctl;

	/* Enable DDR peripheral (disabled by default) */
	ddr_pmd_unlock();

	/* PHY_DLL_RECALIB */
	writel((DELAY_START_VAL << 28) | (RECALIB_CNT << 8), DDR2PHYDLLR);

	phy_pad_ctl = (ODT_SEL | (ODT_EN << 1) | (DRIVE_SEL << 2) |
		       (ODT_PD << 4) | (ODT_PU << 6) |
		       (EXTRA_OEN_CLK << 8) | (NOEXT_DLL << 9) |
		       (DLR_DFI_WRCMD << 13) | (HALF_RATE << 14) |
		       (DRVSTR_PFET << 16) | (DRVSTR_NFET << 20) |
		       (RCVR_EN << 28) | (PREAMBLE_DLY << 29));

	/* PHY_PAD_CTRL */
	writel(phy_pad_ctl, DDR2PHYPADCON);

	/* SCL_CONFIG_0 */
	writel(SCL_BURST8 | (SCL_DDR2_CONNECTED << 1) | (RL << 4) |
	       (SCL_ODTCSWW << 24), DDR2SCLCFG0);

	/* SCL_CONFIG_1 */
	writel(SCL_CSEN | (WL << 8), DDR2SCLCFG1);

	/* SCLLAT */
	writel(SCL_CAPCLKDLY | (SCL_DDRCLKDLY << 4), DDR2SCLLAT);
}

void ddr_init(void)
{
	uint32_t v;
	uint32_t wr2prech, rd2prech, wr2rd, wr2rd_csc;
	uint32_t ras2ras, ras2cas, prech2ras;
	uint32_t ba_field, ma_field;

	/* MEM_WIDTH */
	writel(HALF_RATE_MODE << 3, DDR2MEMWIDTH);

	/* Arbiter regs */
	writel(0 * MIN_LIM_WIDTH, DDR2TSEL);
	writel(0x1f, DDR2MINLIM);
	writel(0 * RQST_PERIOD_WIDTH, DDR2TSEL);
	writel(0xff, DDR2REQPRD);
	writel(0 * MIN_CMDACPT_WIDTH, DDR2TSEL);
	writel(0x04, DDR2MINCMD);

	writel(1 * MIN_LIM_WIDTH, DDR2TSEL);
	writel(0x1f, DDR2MINLIM);
	writel(1 * RQST_PERIOD_WIDTH, DDR2TSEL);
	writel(0xff, DDR2REQPRD);
	writel(1 * MIN_CMDACPT_WIDTH, DDR2TSEL);
	writel(0x10, DDR2MINCMD);

	writel(2 * MIN_LIM_WIDTH, DDR2TSEL);
	writel(0x1f, DDR2MINLIM);
	writel(2 * RQST_PERIOD_WIDTH, DDR2TSEL);
	writel(0xff, DDR2REQPRD);
	writel(2 * MIN_CMDACPT_WIDTH, DDR2TSEL);
	writel(0x10, DDR2MINCMD);

	writel(3 * MIN_LIM_WIDTH, DDR2TSEL);
	writel(0x04, DDR2MINLIM);
	writel(3 * RQST_PERIOD_WIDTH, DDR2TSEL);
	writel(0xff, DDR2REQPRD);
	writel(3 * MIN_CMDACPT_WIDTH, DDR2TSEL);
	writel(0x04, DDR2MINCMD);

	writel(4 * MIN_LIM_WIDTH, DDR2TSEL);
	writel(0x04, DDR2MINLIM);
	writel(4 * RQST_PERIOD_WIDTH, DDR2TSEL);
	writel(0xff, DDR2REQPRD);
	writel(4 * MIN_CMDACPT_WIDTH, DDR2TSEL);
	writel(0x04, DDR2MINCMD);

	/* Address Configuration */
	writel((ROW_ADDR_RSHIFT | (BA_RSHFT << 8) | (CS_ADDR_RSHIFT << 16) |
	       (COL_HI_RSHFT << 24) | (SB_PRI << 29)  |
	       (EN_AUTO_PRECH << 30)), DDR2MEMCFG0);
	writel(ROW_ADDR_MASK, DDR2MEMCFG1);
	writel(COL_HI_MASK, DDR2MEMCFG2);
	writel(COL_LO_MASK, DDR2MEMCFG3);
	writel((BANK_ADDR_MASK | (CS_ADDR_MASK << 8)), DDR2MEMCFG4);

	/* Refresh Config */
	writel((div_round_up(tRFI, CTRL_CLK_PERIOD) - 2) |
	       ((div_round_up(tRFC_MIN, CTRL_CLK_PERIOD) - 2) << 16) |
	       (MAX_PEND_REF << 24), DDR2REFCFG);

	/* Power Config */
	writel((ECC_EN | (ERR_CORR_EN << 1) | (EN_AUTO_PWR_DN << 2) |
	       (EN_AUTO_SELF_REF << 3) | (PWR_DN_DLY << 4) |
	       (SELF_REF_DLY << 12) | (PRECH_PWR_DN_ONLY << 22)),
	       DDR2PWRCFG);

	/* Delay Config */
	wr2rd = max(div_round_up(tWTR, CTRL_CLK_PERIOD),
		    div_round_up(tWTR_TCK, 2)) + WL + BL;
	wr2rd_csc = max(wr2rd - 1, 3);
	wr2prech = div_round_up(tWR, CTRL_CLK_PERIOD) + (WL + BL);
	rd2prech = max(div_round_up(tRTP, CTRL_CLK_PERIOD),
		       div_round_up(tRTP_TCK, 2)) + BL - 2;
	ras2ras = max(div_round_up(tRRD, CTRL_CLK_PERIOD),
		      div_round_up(tRRD_TCK, 2)) - 1;
	ras2cas = div_round_up(tRCD, CTRL_CLK_PERIOD) - 1;
	prech2ras = div_round_up(tRP, CTRL_CLK_PERIOD) - 1;

	writel(((wr2rd & 0x0F) | ((wr2rd_csc & 0x0F) << 4) |
	       ((BL - 1) << 8) | (BL << 12) | ((BL - 1) << 16) |
	       ((BL - 1) << 20) | ((BL + 2) << 24) |
	       ((RL - WL + 3) << 28)), DDR2DLYCFG0);

	writel(((tCKE_TCK - 1) | (((div_round_up(tDLLK, 2) - 2) & 0xFF) << 8) |
	       ((tCKE_TCK - 1) << 16) | ((max(tXP_TCK, tCKE_TCK) - 1) << 20) |
	       ((wr2prech >> 4) << 26) | ((wr2rd >> 4) << 27) |
	       ((wr2rd_csc >> 4) << 28) | (((RL + 5) >> 4) << 29) |
	       ((div_round_up(tDLLK, 2) >> 8) << 30)), DDR2DLYCFG1);

	writel((div_round_up(tRP, CTRL_CLK_PERIOD) | (rd2prech << 8) |
	       ((wr2prech & 0x0F) << 12) | (ras2ras << 16) | (ras2cas << 20) |
	       (prech2ras << 24) | ((RL + 3) << 28)), DDR2DLYCFG2);

	writel(((div_round_up(tRAS_MIN, CTRL_CLK_PERIOD) - 1) |
	       ((div_round_up(tRC, CTRL_CLK_PERIOD) - 1) << 8) |
	       ((div_round_up(tFAW, CTRL_CLK_PERIOD) - 1) << 16)),
	       DDR2DLYCFG3);

	/* ODT Config */
	writel(0x00000000, DDR2ODTCFG);
	writel(0x00010000, DDR2ODTENCFG);/* WREN on CS */

	writel(((RL - 3) << 8) | ((WL - 3) << 12) | (2 << 16) | (3 << 20),
	       DDR2ODTCFG);

	/* TODO: resolve differences in NXTDATRQDLY, NXDATAVDLY and RDATENDLY */
	writel(/*(WL - 1)*/2 | (/*((RL + 1) & 0x0F)*/4  << 4) |
	       (/*(RL - 1)*/2 << 16) |
	       (MAX_BURST << 24) | (7 << 28) | (BIG_ENDIAN << 31),
	       DDR2XFERCFG);

	/* DRAM Initialization */
	/* bring CKE high after reset and wait 400 nsec */
	writel(IDLE_NOP, DDR2CMD10);
	writel((0x00 | (0x00 << 8) | (hc_clk_dly(400000) << 11)),
	       DDR2CMD20);

	/* issue precharge all command */
	writel(PRECH_ALL_CMD, DDR2CMD10 + 0x04);
	writel((0x04 | (0x00 << 8) | (hc_clk_dly(tRP + CLK_PERIOD) << 11)),
	       DDR2CMD20 + 0x04);

	/* initialize EMR2 */
	writel(LOAD_MODE_CMD, DDR2CMD10 + 0x08);
	writel((0x00 | (0x02 << 8) | (hc_clk_dly(tMRD * CLK_PERIOD) << 11)),
	       DDR2CMD20 + 0x08);

	/* initialize EMR3 */
	writel(LOAD_MODE_CMD, DDR2CMD10 + 0x0C);
	writel((0x00 | (0x03 << 8) | (hc_clk_dly(tMRD * CLK_PERIOD) << 11)),
	       DDR2CMD20 + 0x0C);

	/*
	 * RDQS disable, DQSB enable, OCD exit, 150 ohm termination,
	 * AL=0, DLL enable
	 */
	writel((LOAD_MODE_CMD | (0x40 << 24)), DDR2CMD10 + 0x10);
	writel((0x00 | (0x01 << 8) | (hc_clk_dly(tMRD * CLK_PERIOD) << 11)),
	       DDR2CMD20 + 0x10);

	v = ((div_round_up(tWR, CLK_PERIOD) - 1) << 1) | 1;
	ma_field = v & 0xFF;
	ba_field = (v >> 8) & 0x03;

	/*
	 * PD fast exit, WR REC = tWR in clocks -1,
	 * DLL reset, CAS = RL, burst = 4
	 */
	writel((LOAD_MODE_CMD | (((RL << 4) | 2) << 24)), DDR2CMD10 + 0x14);
	writel((ma_field | (ba_field << 8) |
	       (hc_clk_dly(tMRD * CLK_PERIOD) << 11)), DDR2CMD20 + 0x14);

	/* issue precharge all command */
	writel(PRECH_ALL_CMD, DDR2CMD10 + 0x18);
	writel((0x04 | (0x00 << 8) | (hc_clk_dly(tRP + CLK_PERIOD) << 11)),
	       DDR2CMD20 + 0x18);

	/* issue refresh command */
	writel(REF_CMD, DDR2CMD10 + 0x1C);
	writel((0x00 | (0x00 << 8) | (hc_clk_dly(tRFC_MIN) << 11)),
	       DDR2CMD20 + 0x1C);

	/* issue refresh command */
	writel(REF_CMD, DDR2CMD10 + 0x20);
	writel((0x00 | (0x00 << 8) | (hc_clk_dly(tRFC_MIN) << 11)),
	       DDR2CMD20 + 0x20);

	/* Mode register programming as before without DLL reset */
	writel((((RL << 4) | 3) << 24) | LOAD_MODE_CMD, DDR2CMD10 + 0x24);

	v = ((div_round_up(tWR, CLK_PERIOD) - 1) << 1);
	ma_field = v & 0xFF;
	ba_field = (v >> 8) & 0x03;

	writel((ma_field | (ba_field << 8) |
	       (hc_clk_dly(tMRD * CLK_PERIOD) << 11)), DDR2CMD20 + 0x24);

	/* extended mode register same as before with OCD default */
	writel((LOAD_MODE_CMD | (0xC0 << 24)), DDR2CMD10 + 0x28);
	writel((0x03 | (0x01 << 8) | (hc_clk_dly(tMRD * CLK_PERIOD) << 11)),
	       DDR2CMD20 + 0x28);

	/* extended mode register same as before with OCD exit */
	writel((LOAD_MODE_CMD | (0x40 << 24)), DDR2CMD10 + 0x2C);
	writel((0x00 | (0x01 << 8) | (hc_clk_dly(140 * CLK_PERIOD) << 11)),
	       DDR2CMD20 + 0x2C);

	writel(0x1B, DDR2CMDISSUE);
	writel(0x01, DDR2MEMCON);

	while (readl(DDR2CMDISSUE) & 0x10)
		;

	writel(0x03, DDR2MEMCON);

	/* SCL Start */
	writel(SCL_START|SCL_EN, DDR2SCLSTART);

	/* Wait for SCL byte to pass */
	while ((readl(DDR2SCLSTART) & SCL_LUBPASS) != SCL_LUBPASS)
		;
}
