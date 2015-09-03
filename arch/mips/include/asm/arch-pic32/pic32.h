/*
 * (c) 2013 Paul Thacker paul.thacker@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#ifndef PIC32_REGS_H
#define	PIC32_REGS_H

#define _CLR_OFFSET	(0x4)
#define _SET_OFFSET	(0x8)
#define _INV_OFFSET	(0xc)

/* System Configuration */
#define PIC32_BASE_CONFIG 0xbf800000
#define CFGCON		(PIC32_BASE_CONFIG)
#define DEVID		(PIC32_BASE_CONFIG + 0x0020)
#define SYSKEY		(PIC32_BASE_CONFIG + 0x0030)
#define PMD1		(PIC32_BASE_CONFIG + 0x0040)
#define PMD7		(PIC32_BASE_CONFIG + 0x00a0)
#define CFGEBIA		(PIC32_BASE_CONFIG + 0x00c0)
#define CFGEBIC		(PIC32_BASE_CONFIG + 0x00d0)
#define CFGPG		(PIC32_BASE_CONFIG + 0x00e0)
#define CFGMPLL		(PIC32_BASE_CONFIG + 0x0100)

/* Oscillator Configuration */
#define OSC_BASE	0xbf800000
#define OSCCON		(OSC_BASE + 0x1200)
#define SPLLCON		(OSC_BASE + 0x1220)
#define REFO1CON	(OSC_BASE + 0x1280)
#define REFO1TRIM	(OSC_BASE + 0x1290)
#ifdef CONFIG_TARGET_PIC32MZSK
#define PB1DIV		(OSC_BASE + 0x1300)
#else
#define PB1DIV		(OSC_BASE + 0x1340)
#endif

/* Reset Control Registers */
#define RESET_BASE	0xbf800000
#define RSWRST		(RESET_BASE + 0x1250)

/* UART Control */
#define UART_BASE	(0xbf820000 + 0x2000)
#define U_BASE(x)	(UART_BASE + ((x - 1) * 0x200))

#define U_MODE(x)	U_BASE(x)
#define U_MODECLR(x)	(U_MODE(x) + _CLR_OFFSET)
#define U_MODESET(x)	(U_MODE(x) + _SET_OFFSET)
#define U_MODEINV(x)	(U_MODE(x) + _INV_OFFSET)
#define U_STA(x)	(U_BASE(x) + 0x10)
#define U_STACLR(x)	(U_STA(x) + _CLR_OFFSET)
#define U_STASET(x)	(U_STA(x) + _SET_OFFSET)
#define U_STAINV(x)	(U_STA(x) + INV_OFFET)
#define U_TXREG(x)	(U_BASE(x) + 0x20)
#define U_RXREG(x)	(U_BASE(x) + 0x30)
#define U_BRG(x)	(U_BASE(x) + 0x40)
#define U_BRGCLR(x)	(U_BRG(x) + _CLR_OFFSET)
#define U_BRGSET(x)	(U_BRG(x) + _SET_OFFSET)
#define U_BRGINV(x)	(U_BRG(x) + _INV_OFFSET)

/* Peripheral PORTA-PORTK / PORT0-PORT9 */
enum {
	PIC32_PORT_A = 0,
	PIC32_PORT_B = 1,
	PIC32_PORT_C = 2,
	PIC32_PORT_D = 3,
	PIC32_PORT_E = 4,
	PIC32_PORT_F = 5,
	PIC32_PORT_G = 6,
	PIC32_PORT_H = 7,
	PIC32_PORT_J = 8, /* no PORT_I */
	PIC32_PORT_K = 9
};

/* Peripheral Pin Select Input */
#define PPS_IN_BASE	0xbf800000
#define U1RXR		(PPS_IN_BASE + 0x1468)
#define U2RXR		(PPS_IN_BASE + 0x1470)
#define SDI1R		(PPS_IN_BASE + 0x149c)
#define SDI2R		(PPS_IN_BASE + 0x14a8)

/* Peripheral Pin Select Output */
#define PPS_OUT_BASE	0xbf801500
#define PPS_OUT(prt, pi)(PPS_OUT_BASE + ((((prt) * 16) + (pi)) << 2))
#define RPA14R		PPS_OUT(PIC32_PORT_A, 14)
#define RPB0R		PPS_OUT(PIC32_PORT_B, 0)
#define RPB14R		PPS_OUT(PIC32_PORT_B, 14)
#define RPD0R		PPS_OUT(PIC32_PORT_D, 0)
#define RPD3R		PPS_OUT(PIC32_PORT_D, 3)
#define RPG8R		PPS_OUT(PIC32_PORT_G, 8)
#define RPG9R		PPS_OUT(PIC32_PORT_G, 9)

/* Peripheral Pin Control */
#define PIC32_BASE_PORT	0xbf860000
#define ANSEL(x)	(PIC32_BASE_PORT + (x * 0x0100) + 0x00)
#define ANSELCLR(x)	(ANSEL(x) + _CLR_OFFSET)
#define ANSELSET(x)	(ANSEL(x) + _SET_OFFSET)
#define TRIS(x)		(PIC32_BASE_PORT + (x * 0x0100) + 0x10)
#define TRISCLR(x)	(TRIS(x) + _CLR_OFFSET)
#define TRISSET(x)	(TRIS(x) + _SET_OFFSET)
#define PORT(x)		(PIC32_BASE_PORT + (x * 0x0100) + 0x20)
#define PORTCLR(x)	(PORT(x) + _CLR_OFFSET)
#define PORTSET(x)	(PORT(x) + _SET_OFFSET)
#define LAT(x)		(PIC32_BASE_PORT + (x * 0x0100) + 0x30)
#define LATCLR(x)	(LAT(x) + _CLR_OFFSET)
#define LATSET(x)	(LAT(x) + _SET_OFFSET)
#define ODC(x)		(PIC32_BASE_PORT + (x * 0x0100) + 0x40)
#define ODCCLR(x)	(ODC(x) + _CLR_OFFSET)
#define ODCSET(x)	(ODC(x) + _SET_OFFSET)
#define CNPU(x)		(PIC32_BASE_PORT + (x * 0x0100) + 0x50)
#define CNPUCLR(x)	(CNPU(x) + _CLR_OFFSET)
#define CNPUSET(x)	(CNPU(x) + _SET_OFFSET)
#define CNPD(x)		(PIC32_BASE_PORT + (x * 0x0100) + 0x60)
#define CNPDCLR(x)	(CNPD(x) + _CLR_OFFSET)
#define CNPDSET(x)	(CNPD(x) + _SET_OFFSET)
#define CNCON(x)	(PIC32_BASE_PORT + (x * 0x0100) + 0x70)
#define CNCONCLR(x)	(CNCON(x) + _CLR_OFFSET)
#define CNCONSET(x)	(CNCON(x) + _SET_OFFSET)

#define GPIO_PORT_PIN(_port, _pin) (((_port) << 4) + (_pin))

/* USB Core */
#define PIC32_BASE_USB_CORE	0xbf8e3000
#define PIC32_BASE_USB_CTRL	0xbf884000

/* SPI1-SPI6 */
#define PIC32_BASE_SPI1		0xbf821000

/* Prefetch Module */
#define PREFETCH_BASE	0xbf8e0000
#define PRECON		(PREFETCH_BASE)
#define PRECONCLR	(PRECON + _CLR_OFFSET)
#define PRECONSET	(PRECON + _SET_OFFSET)
#define PRECONINV	(PRECON + _INV_OFFSET)

#define PRESTAT		(PREFETCH_BASE + 0x0010)
#define PRESTATCLR	(PRESTAT + _CLR_OFFSET)
#define PRESTATSET	(PRESTAT + _SET_OFFSET)
#define PRESTATINV	(PRESTAT + _INV_OFFSET)

/* Ethernet */
#define ETHCON1_BASE		0xbf882000

/* DDR2 Controller */
#define PIC32_DDR2C_BASE	0xbf8e8000

/* DDR2 PHY */
#define PIC32_DDR2P_BASE	0xbf8e9100

/* EBI */
#define PIC32_BASE_EBI		0xbf8e1000

/* SDHC */
#define SDHC_BASE		0xbf8ec000
#define SDHC_SHARED_BUS_CTRL	0x000000e0
#define SDHC_MIN_CLK		25000000
#define SDHC_MAX_CLK		25000000

/* SQI */
#define SQI_BASE		0xbf8e2000
#define SQI_MAX_CLK		50000000

#endif	/* PIC32_REGS_H */
