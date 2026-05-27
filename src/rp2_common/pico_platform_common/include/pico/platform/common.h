/*
* Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_PLATFORM_COMMON_H
#define _PICO_PLATFORM_COMMON_H

/** \file pico/platform/common.h
 *  \ingroup pico_platform
 *
 * \brief Macros and definitions common to all rp2 platforms but not specific to any library
 *
 * This header may be included by assembly code
 *
 * Note certain library specific defines are defined here when they are interdpedent across libraries,
 * but making an explicit library dependency does not make sense.
 */

// PICO_CONFIG: PICO_MINIMAL_STORED_VECTOR_TABLE, Only store a very minimal vector table in the binary on Arm, type=bool, default=0, advanced=true, group=pico_crt0
#ifndef PICO_MINIMAL_STORED_VECTOR_TABLE
#define PICO_MINIMAL_STORED_VECTOR_TABLE 0
#endif

#if PICO_MINIMAL_STORED_VECTOR_TABLE && (PICO_NO_FLASH && !defined(__riscv))
#if PICO_NUM_VTABLE_IRQS
#warning PICO_NUM_VTABLE_IRQS is specied with PICO_MINIMAL_STORED_VECTOR_TABLE for NO_FLASH Arm binary; ignored
#undef PICO_NUM_VTABLE_IRQS
#endif
#define PICO_NUM_VTABLE_IRQS 0
#else
// PICO_CONFIG: PICO_NUM_VTABLE_IRQS, Number of IRQ handlers in the vector table - can be lowered to save space if you aren't using some higher IRQs, type=int, default=NUM_IRQS, group=hardware_irq
#ifndef PICO_NUM_VTABLE_IRQS
#define PICO_NUM_VTABLE_IRQS NUM_IRQS
#endif
#endif

#ifndef __ASSEMBLER__

// PICO_CONFIG: PICO_NO_FPGA_CHECK, Remove the FPGA platform check for small code size reduction, type=bool, default=1, advanced=true, group=pico_runtime
#ifndef PICO_NO_FPGA_CHECK
#define PICO_NO_FPGA_CHECK 1
#endif

// PICO_CONFIG: PICO_NO_SIM_CHECK, Remove the SIM platform check for small code size reduction, type=bool, default=1, advanced=true, group=pico_runtime
#ifndef PICO_NO_SIM_CHECK
#define PICO_NO_SIM_CHECK 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if PICO_NO_FPGA_CHECK
static inline bool running_on_fpga(void) {return false;}
#else
bool running_on_fpga(void);
#endif

#if PICO_NO_SIM_CHECK
static inline bool running_in_sim(void) {return false;}
#else
bool running_in_sim(void);
#endif

/*! \brief No-op function for the body of tight loops
 *  \ingroup pico_platform
 *
 * No-op function intended to be called by any tight hardware polling loop. Using this ubiquitously
 * makes it much easier to find tight loops, but also in the future \#ifdef-ed support for lockup
 * debugging might be added
 */
static __force_inline void tight_loop_contents(void) {}

/*! \brief RP2 chip version enum
 *  \ingroup pico_platform
 * 
 * Each value is formed of the following bits:
 * bits 0-7: Chip version in ROM (\see rp2_get_rom_chip_version)
 * bits 8-15: Chip revision (\see rp2_get_chip_revision)
 * bits 16-23: ROM version (\see rp2_get_rom_version)
 */
typedef enum rp2_chip {
    chip_rp2040_b0 = 0x010101,
    chip_rp2040_b1 = 0x020101,
    chip_rp2040_b2 = 0x030201,
    chip_rp2350_a2 = 0x020202,
    chip_rp2350_a3 = 0x030302,
    chip_rp2350_a4 = 0x040302,
} rp2_chip_t;

/*! \brief Get RP2 bootrom version number
 *  \ingroup pico_platform
 *
 * Reads the 1 byte bootrom version from 0x13
 * 
 * \return the bootrom version
 */
uint8_t rp2_get_rom_version(void);

/*! \brief Get RP2 bootrom magic
 *  \ingroup pico_platform
 *
 * Reads the 3 byte bootrom magic from 0x10, which should be `M`,`u` followed by the chip version
 * (0x01 for RP2040, 0x02 for RP2350)
 *
 * \return the bootrom magic
 */
uint32_t rp2_get_rom_magic(void);

/*! \brief Get RP2 bootrom chip version number
 *  \ingroup pico_platform
 *
 * Reads the last byte of the bootrom magic, and asserts the first 2 bytes are `M`,`u`
 *
 * \return the bootrom chip version
 */
uint8_t rp2_get_rom_chip_version(void);

/*! \brief Get the RP2 chip revision number
 *  \ingroup pico_platform
 *
 * Reads the chip revision from SYSINFO
 * 
 * @return the chip revision number
 */
uint8_t rp2_get_chip_revision(void);

/*! \brief Get the RP2 chip
 *  \ingroup pico_platform
 *
 * Uses \ref rp2_get_rom_version, \ref rp2_get_rom_chip_version and \ref rp2_get_chip_revision
 * to determine what chip the code is running on
 *
 * \return the RP2 chip, \see rp2_chip
 */
rp2_chip_t rp2_get_chip(void);

#define host_safe_hw_ptr(x) ((uintptr_t)(x))
#define native_safe_hw_ptr(x) host_safe_hw_ptr(x)

#ifdef __cplusplus
}
#endif
#endif // __ASSEMBLER__


#endif