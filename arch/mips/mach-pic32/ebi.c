/*
 * Copyright (C) 2015
 * Joshua Henderson, joshua.henderson@microchip.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#include <common.h>
#include <asm/io.h>

#include <asm/arch/pic32.h>
#include <asm/arch-pic32/ap.h>

#ifndef CONFIG_PIC32_NO_EBI_SRAM
/* EBI Registers */
#define EBICS0		0x14
#define EBICS1		0x18
#define EBICS2		0x1C
#define EBICS3		0x20
#define EBIMSK0		0x54
#define EBIMSK1		0x58
#define EBIMSK2		0x5C
#define EBIMSK3		0x60
#define EBISMT0		0x94
#define EBISMT1		0x98
#define EBISMT2		0x9C
#define EBISMT3		0xA0
#define EBISMCON	0xA4

/* EBIMSKx Register fields */
#define EBI_SMT0	0x0000
#define EBI_SMT1	0x0100
#define EBI_SMT2	0x0200

#define EBI_NOR_FLASH	0x40
#define EBI_SRAM	0x20
#define EBI_MEMSZ_1M	0x05
#define EBI_MEMSZ_2M	0x06
#define EBI_MEMSZ_4M	0x07
#define EBI_MEMSZ_8M	0x08
#define EBI_MEMSZ_16M	0x09

void setup_ebi_sram(void)
{
	void *ebi_base = (void __iomem *)PIC32_BASE_EBI;

	/*
	 * Enable address lines [0:23]
	 * Controls access of pins shared with PMP
	 */
	writel(0x80FFFFFF, CFGEBIA);

	/*
	 * Enable write enable pin
	 * Enable output enable pin
	 * Enable byte select pin 0
	 * Enable byte select pin 1
	 * Enable byte select pin 2
	 * Enable byte select pin 3
	 * Enable Chip Select 0
	 * Enable data pins [0:15]
	 */
	writel(0x000033F3, CFGEBIC);

	/*
	 * Connect CS0/CS1/CS2/CS3 to physical address
	 */
#if defined(CONFIG_SYS_PIC32_PLANB)
	writel(0x20000000, ebi_base + EBICS0);
	writel(0x20200000, ebi_base + EBICS1);
	writel(0x20400000, ebi_base + EBICS2);
	writel(0x20600000, ebi_base + EBICS3);
#elif defined(CONFIG_SYS_PIC32_PLANC) || defined(CONFIG_SYS_PIC32_PLAND)
	/* PLANC/D daughter board populated with 4M SRAM chip */
	writel(0x20000000, ebi_base + EBICS0);
	writel(0x20400000, ebi_base + EBICS1);
	writel(0x20800000, ebi_base + EBICS2);
	writel(0x20C00000, ebi_base + EBICS3);
#endif
	/*
	 * Memory size is set as 2 MB
	 * Memory type is set as SRAM
	 * Uses timing numbers from EBISMT0-1
	 */
#if defined(CONFIG_SYS_PIC32_PLANB)
	writel(EBI_SRAM|EBI_MEMSZ_2M, ebi_base + EBIMSK0);
	writel(EBI_SRAM|EBI_MEMSZ_2M, ebi_base + EBIMSK1);
	writel(EBI_SRAM|EBI_MEMSZ_2M, ebi_base + EBIMSK2);
	writel(EBI_SRAM|EBI_MEMSZ_2M, ebi_base + EBIMSK3);
#elif defined(CONFIG_SYS_PIC32_PLANC)
	writel(EBI_SRAM|EBI_MEMSZ_4M, ebi_base + EBIMSK0);
	writel(EBI_SRAM|EBI_MEMSZ_4M, ebi_base + EBIMSK1);
	writel(EBI_SRAM|EBI_MEMSZ_4M, ebi_base + EBIMSK2);
	writel(EBI_SRAM|EBI_MEMSZ_4M, ebi_base + EBIMSK3);
#elif defined(CONFIG_SYS_PIC32_PLAND)
	writel(EBI_SMT0|EBI_SRAM|EBI_MEMSZ_4M, ebi_base + EBIMSK0);
	writel(EBI_SMT0|EBI_SRAM|EBI_MEMSZ_4M, ebi_base + EBIMSK1);
	writel(EBI_SMT1|EBI_SRAM|EBI_MEMSZ_4M, ebi_base + EBIMSK2);
	writel(EBI_SMT1|EBI_SRAM|EBI_MEMSZ_4M, ebi_base + EBIMSK3);
#endif
	/*
	 * Configure EBISMT0
	 * ISSI device has read cycles time of 6 ns
	 * ISSI device has address setup time of 0ns
	 * ISSI device has address/data hold time of 2.5 ns
	 * ISSI device has Write Cycle Time of 6 ns
	 * Bus turnaround time is 0 ns
	 * No page mode
	 * No page size
	 * No RDY pin
	 */
#if defined(CONFIG_SYS_PIC32_PLANB)
	writel(0x2|(1 << 6)|(1 << 8)|(1 << 10), ebi_base + EBISMT0);
#elif defined(CONFIG_SYS_PIC32_PLANC)
	writel(0x7|(1 << 6)|(1 << 8)|(2 << 10), ebi_base + EBISMT0);
#elif defined(CONFIG_SYS_PIC32_PLAND)
	writel(0x3|(1 << 6)|(1 << 8)|(1 << 10), ebi_base + EBISMT0);
	writel(0x4|(1 << 6)|(1 << 8)|(2 << 10), ebi_base + EBISMT1);
#endif
	/*
	 * Keep default data width to 16-bits
	 */
	writel(0x00000000, ebi_base + EBISMCON);
}
#endif

#ifdef CONFIG_SYS_DRAM_TEST
static void write_pattern(u32 p, u32 size, void *base)
{
	u32 *addr = (u32 *)base;
	u32 loop;

	printf("mem: write test pattern 0x%x ...", p);

	for (loop = 0; loop < size / 4; loop++)
		*addr++ = p;

	printf("finished\n");
}

static void read_pattern(u32 p, u32 size, void *base)
{
	u32 *addr = (u32 *)base;
	u32 loop;
	u32 val;

	printf("mem: read test pattern 0x%x ...", p);

	for (loop = 0 ; loop < size / 4; loop++) {
		val = *addr++;
		if (val != p) {
			printf("pattern 0x%x failed at %p\n",
			       p, (void *)addr);
			panic("mem: is bad");
		}
	}
	printf("success\n");
}
#endif
void run_memory_test(u32 size, void *base)
{
#ifdef CONFIG_SYS_DRAM_TEST
	u32 *addr;
	u32 loop;
	u32 val;
	u32 count = 0;

	printf("mem: running pattern test on [%p - %p]\n",
	       base, base + size - 1);

	write_pattern(0xFFFFFFFF, size, base);
	read_pattern(0xFFFFFFFF, size, base);

	write_pattern(0xa5a5a5a5, size, base);
	read_pattern(0xa5a5a5a5, size, base);

	write_pattern(0x5a5a5a5a, size, base);
	read_pattern(0x5a5a5a5a, size, base);

	write_pattern(0x5555aaaa, size, base);
	read_pattern(0x5555aaaa, size, base);

	write_pattern(0xaaaaaaaa, size, base);
	read_pattern(0xaaaaaaaa, size, base);

	write_pattern(0x55555555, size, base);
	read_pattern(0x55555555, size, base);

	write_pattern(0x33333333, size, base);
	read_pattern(0x33333333, size, base);

	write_pattern(0xcccccccc, size, base);
	read_pattern(0xcccccccc, size, base);

	write_pattern(0x0F0F0F0F, size, base);
	read_pattern(0x0F0F0F0F, size, base);

	write_pattern(0xF0F0F0F0, size, base);
	read_pattern(0xF0F0F0F0, size, base);

	write_pattern(0xFF00FF00, size, base);
	read_pattern(0xFF00FF00, size, base);

	write_pattern(0x00FF00FF, size, base);
	read_pattern(0x00FF00FF, size, base);

	write_pattern(0x0, size, base);
	read_pattern(0x0, size, base);

	printf("mem: write test running...");


	/* address test */
	count = 0;
	addr = (u32 *)base;
	for (loop = 0; loop < size / 4; loop++)
		*addr++ = count++;

	printf("finished\n");
	printf("mem: read test running...");

	count = 0;
	addr = (u32 *)base;
	for (loop = 0 ; loop < size / 4; loop++) {
		val = *addr++;
		if (val != count) {
			printf("failed at 0x%x: 0x%x != 0x%x\n",
			       loop * 4, val, count);
			panic("mem: is bad");
		}
		count++;
	}
	printf("success\n");
#endif
}

