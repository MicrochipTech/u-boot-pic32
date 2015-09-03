/*
 * Copyright (C) 2015
 * Cristian Birsan <cristian.birsan@microchip.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#include <common.h>
#include <flash.h>
#include <asm/io.h>
#include <linux/byteorder/swab.h>

#if defined(CONFIG_ENV_IS_IN_FLASH)
#ifndef CONFIG_ENV_ADDR
#define CONFIG_ENV_ADDR	(CONFIG_SYS_FLASH_BASE + CONFIG_ENV_OFFSET)
#endif

#ifndef CONFIG_ENV_SIZE
#define CONFIG_ENV_SIZE	CONFIG_ENV_SECT_SIZE
#endif

#ifndef CONFIG_ENV_SECT_SIZE
#define CONFIG_ENV_SECT_SIZE  CONFIG_ENV_SIZE
#endif
#endif

/* NVM Controller registers */
#define NVMCON		0xBF800600
#define NVMCONCLR	0xBF800604
#define NVMCONSET	0xBF800608
#define NVMCONINV	0xBF80060C
#define NVMKEY		0xBF800610
#define NVMADDR		0xBF800620
#define NVMADDRCLR	0xBF800624
#define NVMADDRSET	0xBF800628
#define NVMADDRINV	0xBF80062C
#define NVMDATA0	0xBF800630
#define NVMDATA1	0xBF800640
#define NVMDATA2	0xBF800650
#define NVMDATA3	0xBF800660
#define NVMSRCADDR	0xBF800670
#define NVMPWP		0xBF800680
#define NVMBWP		0xBF800690
#define NVMBWPCLR	0xBF800694
#define NVMBWPSET	0xBF800698
#define NVMBWPINV	0xBF80069C

/* NVM Operations */
#define NVMOP_NOP		0x00000000
#define	NVMOP_WORD_WRITE	0x00000001
#define NVMOP_QUAD_WORD_WRITE	0x00000002
#define NVMOP_ROW_WRITE		0x00000003
#define NVMOP_PAGE_ERASE	0x00000004
#define NVMOP_LO_BANK_ERASE	0x00000005
#define NVMOP_HI_BANK_ERASE	0x00000006
#define NVMOP_ALL_BANKS_ERASE	0x00000007

/* NVM Programming Control Register*/
#define NVMCON_WREN		0x00004000
#define	NVMCON_WR		0x00008000
#define NVMCON_WRERR		0x00002000
#define NVMCON_LVDERR		0x00001000

/*-----------------------------------------------------------------------
 */
flash_info_t flash_info[CONFIG_SYS_MAX_FLASH_BANKS];

/*
 * The following code cannot be run from FLASH!
 */
static ulong flash_get_size(vu_long *addr, flash_info_t *info)
{
	short i;
	ulong base = (ulong)addr;
	ulong sector_offset;

	/* On chip flash ID */
	switch (info->flash_id & FLASH_VENDMASK) {
	case FLASH_MAN_MCHP:
		/* printf("Microchip "); */
		break;
	default:
		/* no or unknown flash	*/
		printf("unknown manufacturer: 0x%lx\n",
		       info->flash_id & FLASH_VENDMASK);
		info->flash_id = FLASH_UNKNOWN;
		info->sector_count = 0;
		info->size = 0;
		return 0;
	}

	switch (info->flash_id & FLASH_TYPEMASK) {
	case FLASH_MCHP100T:
		info->sector_count = CONFIG_SYS_MAX_FLASH_SECT;
		info->size = CONFIG_SYS_FLASH_SIZE;
		sector_offset = info->size / info->sector_count;
		break;
	default:
		info->flash_id = FLASH_UNKNOWN;
		return 0;			/* => no or unknown flash */
	}

	/* set up sector start address table */
	for (i = 0; i < info->sector_count; i++) {
		info->start[i] = base;
		base += sector_offset;
		/* protect each sector by default */
		info->protect[i] = 1;
	}

	/* Disable Flash Write/Erase operations */
	writel(NVMCON_WREN, NVMCONCLR);

	if (info->flash_id != FLASH_UNKNOWN)
		addr = (vu_long *)info->start[0];

	return info->size;
}


/*-----------------------------------------------------------------------
 */
void flash_print_info(flash_info_t *info)
{
	int i;

	if (info->flash_id == FLASH_UNKNOWN) {
		printf("missing or unknown FLASH type\n");
		return;
	}

	switch (info->flash_id & FLASH_VENDMASK) {
	case FLASH_MAN_MCHP:
		printf("Microchip ");
		break;
	default:
		printf("Unknown Vendor ");
		break;
	}

	switch (info->flash_id & FLASH_TYPEMASK) {
	case FLASH_MCHP100T:
		printf("Internal (8 Mbit, 64 x 16k)\n");
		break;
	default:
		printf("Unknown Chip Type\n");
		break;
	}

	printf("  Size: %ld MB in %d Sectors\n",
	       info->size >> 20, info->sector_count);

	printf("  Sector Start Addresses:");
	for (i = 0; i < info->sector_count; ++i) {
		if ((i % 5) == 0)
			printf("\n   ");

		printf(" %08lX%s", info->start[i],
		       info->protect[i] ? " (RO)" : "     "
		);
	}
	printf("\n");
}


/*-----------------------------------------------------------------------
 */

static inline void flash_initiate_operation(void)
{
	/* Unlock sequence */
	writel(0x00000000, NVMKEY);
	writel(0xAA996655, NVMKEY);
	writel(0x556699AA, NVMKEY);

	writel(NVMCON_WR, NVMCON);
}

static inline void flash_nop_operation(void)
{
	/* reset error bits using a flash NOP command */

	writel(NVMOP_NOP, NVMCON); /* NVMOP for page erase*/
	writel(NVMCON_WREN, NVMCONSET); /* Enable Flash Write*/
	flash_initiate_operation();
}

int flash_erase(flash_info_t *info, int s_first, int s_last)
{
	int flag, prot, sect;
	ulong base, elapsed, last = 0, tmp, addr;

	if ((info->flash_id & FLASH_VENDMASK) != FLASH_MAN_MCHP) {
		printf("Can't erase unknown flash type %08lx - aborted\n",
		       info->flash_id);
		return ERR_UNKNOWN_FLASH_VENDOR;
	}

	if ((s_first < 0) || (s_first > s_last)) {
		printf("- no sectors to erase\n");
		return ERR_INVAL;
	}

	prot = 0;
	for (sect = s_first; sect <= s_last; ++sect) {
		if (info->protect[sect])
			prot++;
	}

	if (prot)
		printf("- Warning: %d protected sectors will not be erased!\n",
		       prot);
	else
		printf("\n");

	base = get_timer(0);

	/* Start erase on unprotected sectors */
	for (sect = s_first; sect <= s_last; sect++) {
		if (info->protect[sect]) /* skip protected sector */
			continue;

		/* Disable interrupts which might cause timeout */
		flag = disable_interrupts();

		/* destination page physical address */
		addr = virt_to_phys((void *)info->start[sect]);
		writel(addr, NVMADDR);

		/* NVMOP for page erase*/
		writel(NVMOP_PAGE_ERASE, NVMCON);
		/* Enable Flash Write*/
		writel(NVMCON_WREN, NVMCONSET);

		/* Initiate operation */
		flash_initiate_operation();

		/* Wait for WR bit to clear */
		while (readl(NVMCON) & NVMCON_WR) {
			elapsed = get_timer(base);
			if (elapsed > CONFIG_SYS_FLASH_ERASE_TOUT) {
				printf("Timeout\n");
				/* reset bank */
				return ERR_TIMOUT;
			}

			/* show that we're waiting */
			if ((elapsed - last) > 100) { /* every 100msec */
				putc('.');
				last = elapsed;
			}
		}

		tmp = readl(NVMCON);
		if (tmp & NVMCON_WRERR) {
			printf("Error in Block Erase - Lock Bit may be set!\n");
			flash_nop_operation();
			return ERR_PROTECTED;
		}

		if (tmp & NVMCON_LVDERR) {
			printf("Error in Block Erase - low-vol detected!\n");
			flash_nop_operation();
			return ERR_NOT_ERASED;
		}

		/* Disable future Flash Write/Erase operations */
		writel(NVMCON_WREN, NVMCONCLR);

		/* re-enable interrupts if necessary */
		if (flag)
			enable_interrupts();
	}

	for (sect = s_first; sect <= s_last; sect++) {
		addr = info->start[sect];
		tmp = addr + (info->size / info->sector_count);
		invalidate_dcache_range(addr, tmp);
	}

	printf(" done\n");
	return ERR_OK;
}

int page_erase(flash_info_t *info, int sect)
{
	return 0;
}

/*-----------------------------------------------------------------------
 * Write a word to Flash, returns:
 * 0 - OK
 * 1 - write timeout
 * 2 - Flash not erased
 */
static int write_word(flash_info_t *info, ulong dest, ulong data)
{
	vu_long *addr = (vu_long *)dest;
	ulong base, elapsed, last = 0, tmp;
	int rc;

	/* Check if Flash is (sufficiently) erased */
	if ((*addr & data) != data) {
		printf("Error, Flash not erased!\n");
		return ERR_NOT_ERASED;
	}

	base = get_timer(0);

	/* Disable interrupts which might cause a timeout here */
	rc = disable_interrupts();

	/* destination page physical address*/
	writel(virt_to_phys(addr), NVMADDR);
	writel(data, NVMDATA0);

	/* NVMOP for word write*/
	writel(NVMOP_WORD_WRITE, NVMCON);

	/* Enable Flash Write*/
	writel(NVMCON_WREN, NVMCONSET);

	/* Initiate operation */
	flash_initiate_operation();

	/* re-enable interrupts if necessary */
	if (rc)
		enable_interrupts();

	/* Wait for WR bit to clear */
	while (readl(NVMCON) & NVMCON_WR) {
		elapsed = get_timer(base);
		if (elapsed > CONFIG_SYS_FLASH_WRITE_TOUT) {
			printf("Timeout\n");
			/* reset bank */
			return ERR_TIMOUT;
		}

		/* show that we're waiting */
		if ((elapsed - last) > 10) {	/* every 10msec */
			putc('.');
			last = elapsed;
		}
	}

	rc = 0;
	tmp = readl(NVMCON);
	if (tmp & NVMCON_WRERR) {
		printf("Error in Block Write - Flash may be locked !\n");
		flash_nop_operation();
		rc |= ERR_PROG_ERROR;
	}

	if (tmp & NVMCON_LVDERR) {
		printf("Error in Block Write - Brown out Reset detected!\n");
		flash_nop_operation();
		rc |= ERR_ABORTED;
	}

	/* Disable future Flash Write/Erase operations */
	writel(NVMCON_WREN, NVMCONCLR);

	return rc;
}

/*-----------------------------------------------------------------------
 * Copy memory to flash, returns:
 * 0 - OK
 * 1 - write timeout
 * 2 - Flash not erased
 */

int write_buff(flash_info_t *info, uchar *src, ulong addr, ulong cnt)
{
	ulong cp, wp, data, n = cnt;
	int i, l, rc;

	wp = (addr & ~3);	/* get lower word aligned address */

	/*
	 * handle unaligned start bytes
	 */
	l = addr - wp;
	if (l != 0) {
		data = 0;
		for (i = 0, cp = wp; i < l; ++i, ++cp)
			data = (data << 8) | (*(uchar *)cp);

		for (; (i < 4) && (cnt > 0); ++i) {
			data = (data << 8) | *src++;
			--cnt;
			++cp;
		}

		for (; (cnt == 0) && (i < 4); ++i, ++cp)
			data = (data << 8) | (*(uchar *)cp);


		rc = write_word(info, wp, __swab32(data));
		if (rc)
			goto out;

		wp += 4;
	}

	/*
	 * handle word aligned part
	 */
	while (cnt >= 4) {
		data = 0;
		for (i = 0; i < 4; ++i)
			data = (data << 8) | *src++;

		rc = write_word(info, wp, __swab32(data));
		if (rc)
			goto out;

		wp  += 4;
		cnt -= 4;
	}

	if (cnt == 0) {
		rc = ERR_OK;
		goto out;
	}

	/*
	 * handle unaligned tail bytes
	 */
	data = 0;
	for (i = 0, cp = wp; (i < 4) && (cnt > 0); ++i, ++cp) {
		data = (data << 8) | *src++;
		--cnt;
	}

	for (; i < 4; ++i, ++cp)
		data = (data << 8) | (*(uchar *)cp);

	rc = write_word(info, wp,  __swab32(data));

out:
	invalidate_dcache_range(addr, addr + n);
	return rc;
}

unsigned long flash_init(void)
{
	unsigned long size;
	vu_long *addr;
	int i;

	/* Init: enable write,
	 * or we cannot even write flash commands
	 */

	for (i = 0; i < CONFIG_SYS_MAX_FLASH_BANKS; ++i)
		flash_info[i].flash_id = FLASH_UNKNOWN;

	/* flash info: combined device & manufacturer code  */
	flash_info[0].flash_id = FLASH_MAN_MCHP | FLASH_MCHP100T;
	flash_info[1].flash_id = FLASH_MAN_MCHP | FLASH_MCHP100T;

	/* Static FLASH Bank configuration here */
	addr = (vu_long *)phys_to_virt(PHYS_FLASH_1);
	flash_info[0].size = flash_get_size(addr, &flash_info[0]);
	size = flash_info[0].size;

	addr = (vu_long *)phys_to_virt(PHYS_FLASH_2);
	flash_info[1].size = flash_get_size(addr, &flash_info[1]);
	size += flash_info[1].size;

	for (i = 0; i < CONFIG_SYS_MAX_FLASH_BANKS; i++) {
		if (flash_info[i].flash_id == FLASH_UNKNOWN) {
			printf("## Unknown FLASH on Bank 0 - Size = 0x%08lx = %ld MB\n",
			       size, size << 20);
		}
	}

#if (CONFIG_SYS_MONITOR_BASE >= CONFIG_SYS_FLASH_BASE)
	/* monitor protection ON by default */
	flash_protect(FLAG_PROTECT_SET,
		      CONFIG_SYS_MONITOR_BASE,
		      CONFIG_SYS_MONITOR_BASE + monitor_flash_len - 1,
		      &flash_info[0]);
#endif

#ifdef CONFIG_ENV_IS_IN_FLASH
	/* ENV protection ON by default */
	flash_protect(FLAG_PROTECT_SET,
		      CONFIG_ENV_ADDR,
		      CONFIG_ENV_ADDR + CONFIG_ENV_SECT_SIZE - 1,
		      &flash_info[0]);
#endif
	return size;
}
