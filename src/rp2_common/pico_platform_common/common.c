/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico.h"
#include "hardware/address_mapped.h"
#include "hardware/regs/tbman.h"
#include "hardware/regs/sysinfo.h"

// Note we leave the FPGA check in by default so that we can run bug repro
// binaries coming in from the wild on the FPGA platform. It takes up around
// 48 bytes if you include all the calls, so you can pass PICO_NO_FPGA_CHECK=1
// to remove it. The FPGA check is used to skip initialisation of hardware
// (mainly clock generators and oscillators) that aren't present on FPGA.

#if !PICO_NO_FPGA_CHECK
// Inline stub provided in header if this code is unused (so folding can be
// done in each TU instead of relying on LTO)
bool __attribute__((weak)) running_on_fpga(void) {
    return (*(io_ro_32 *)TBMAN_BASE) & TBMAN_PLATFORM_FPGA_BITS;
}
#endif

#if !PICO_NO_SIM_CHECK
bool __attribute__((weak)) running_in_sim(void) {
    return (*(io_ro_32 *)TBMAN_BASE) & TBMAN_PLATFORM_HDLSIM_BITS;
}
#endif

uint8_t rp2_get_rom_version(void) {
    GCC_Pragma("GCC diagnostic push")
    GCC_Pragma("GCC diagnostic ignored \"-Warray-bounds\"")
    return *(uint8_t*)0x13;
    GCC_Pragma("GCC diagnostic pop")
}

uint32_t rp2_get_rom_magic(void) {
    GCC_Pragma("GCC diagnostic push")
    GCC_Pragma("GCC diagnostic ignored \"-Warray-bounds\"")
    return (*(uint32_t*)0x10) & (0xffffff); // only bottom 3 bytes
    GCC_Pragma("GCC diagnostic pop")
}

uint8_t rp2_get_rom_chip_version(void) {
    assert(('M' | ('u' << 8)) == (rp2_get_rom_magic() & 0xffff));
    return rp2_get_rom_magic() >> 16;
}

uint8_t rp2_get_chip_revision(void) {
    uint32_t chip_id = *((io_ro_32*)(SYSINFO_BASE + SYSINFO_CHIP_ID_OFFSET));
    uint32_t revision = (chip_id & SYSINFO_CHIP_ID_REVISION_BITS) >> SYSINFO_CHIP_ID_REVISION_LSB;
    return (uint8_t)revision;
}

rp2_chip_t rp2_get_chip(void) {
    uint8_t chip_version = rp2_get_rom_chip_version();
    uint8_t chip_revision = rp2_get_chip_revision();
    uint8_t rom_version = rp2_get_rom_version();

    return (rp2_chip_t)(chip_version | (chip_revision << 8) | (rom_version << 16));
}
