/*
 * (c) 2014 Purna Chandra Mandal purna.mandal@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#ifndef __PIC32_AP_H
#define	__PIC32_AP_H

ulong pic32_get_pbclk(int bus);
ulong pic32_get_cpuclk(void);
ulong pic32_get_refclk(int bus);
#define pic32_get_sqiclk() pic32_get_refclk(2)
int init_prefetch(void);
void clk_init(void);
char *get_boardinfo(void);
void setup_ebi_sram(void);
void run_memory_test(u32 size, void *base);
int pic32_sdhci_init(u32 regbase, u32 max_clk, u32 min_clk);
void write_one_tlb(int index, u32 pagemask, u32 hi, u32 low0, u32 low1);

static inline unsigned long pic32_virt_to_uncac(unsigned long addr)
{
	if ((KSEGX(addr) == KSEG2) || (KSEGX(addr) == KSEG3))
		return CKSEG3ADDR(addr);

	return CKSEG1ADDR(addr);
}

static inline unsigned long pic32_virt_to_phys(void *address)
{
	unsigned long ret;

	ret = (unsigned long)virt_to_phys(address);
	if ((KSEGX(address) == KSEG2) || (KSEGX(address) == KSEG3))
		ret |= 0x20000000;

	return ret;
}

#define ENTRYLO_CAC(_pa) (((_pa) >> 6)|(CONFIG_SYS_MIPS_CACHE_MODE << 3)|0x7)
#define ENTRYLO_UNC(_pa) (((_pa) >> 6)|(CONF_CM_UNCACHED << 3)|0x7)

#endif	/* __PIC32_AP_H */
