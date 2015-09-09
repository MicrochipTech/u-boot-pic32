#
# Microchip PIC32MZ[DA] board (PIC32 SoC powered by MIPS M14KEc CPU)
#

# ROM version
CONFIG_SYS_TEXT_BASE = 0x9d004000

# RAM version
# leave 32K(max(dcache_size, icache_size) reserved starting at kseg0/kseg1.
# 'mips_cache_reset' needs this region unused so as to prefill this with zero.
#CONFIG_SYS_TEXT_BASE = 0x80008000
