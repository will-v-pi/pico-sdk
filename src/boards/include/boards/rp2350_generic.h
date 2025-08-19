/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

// This header may be included by other board headers as "boards/rp2350_generic.h"

#ifndef _BOARDS_RP2350_GENERIC_H
#define _BOARDS_RP2350_GENERIC_H

pico_board_cmake_set(PICO_PLATFORM, rp2350)

// Enable CYW43 support by default
pico_board_cmake_set_default(PICO_CYW43_SUPPORTED, 1)

// For board detection
#define RASPBERRYPI_RP2350_GENERIC

// For post-compile configuration
pico_board_cmake_set(PICO_SUPPORT_CONFIGURABLE_PINS, 1)
#define PICO_SUPPORT_CONFIGURABLE_PINS 1

// On some boards, the xosc can take longer to stabilize than is usual
#ifndef PICO_XOSC_STARTUP_DELAY_MULTIPLIER
#define PICO_XOSC_STARTUP_DELAY_MULTIPLIER 64
#endif

// --- RP2350 VARIANT ---

// Support RP2350B (which includes support for RP2350A)
#define PICO_RP2350A 0

// --- FLASH ---

// Use the generic 03H boot stage2 for maximum compatibility
#define PICO_BOOT_STAGE2_CHOOSE_GENERIC_03H 1

// Set default flash SPI clock divider to the maximum value of all RP2350 boards
#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 4
#endif

// Set default flash size to the minimum size of all RP2350 boards
pico_board_cmake_set_default(PICO_FLASH_SIZE_BYTES, (2 * 1024 * 1024))
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024)
#endif

// --- REVISION SUPPORT ---

// Support all RP2350 revisions
pico_board_cmake_set_default(PICO_RP2350_A2_SUPPORTED, 1)
#ifndef PICO_RP2350_A2_SUPPORTED
#define PICO_RP2350_A2_SUPPORTED 1
#endif

#endif
