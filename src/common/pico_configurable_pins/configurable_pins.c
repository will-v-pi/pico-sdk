/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/configurable_pins.h"
#include "pico/binary_info.h"

#if PICO_SUPPORT_CONFIGURABLE_PINS

#define BINARY_INFO_TAG_CONFIGURABLE_PINS BINARY_INFO_MAKE_TAG('C','P')
#define BINARY_INFO_ID_CONFIGURABLE_PINS 0x00000000

bi_decl(bi_program_feature_group(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, "Pin Configuration"));

bi_decl(bi_ptr_global_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_uart, 0));
bi_decl(bi_ptr_global_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_uart_tx_pin, 0));
bi_decl(bi_ptr_global_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_uart_rx_pin, 1));
bi_decl(bi_ptr_global_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_led_pin, 25));
bi_decl(bi_ptr_global_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_i2c, 0));
bi_decl(bi_ptr_global_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_i2c_sda_pin, 4));
bi_decl(bi_ptr_global_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_i2c_scl_pin, 5));
bi_decl(bi_ptr_global_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_spi, 0));
bi_decl(bi_ptr_global_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_spi_sck_pin, 18));
bi_decl(bi_ptr_global_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_spi_tx_pin, 19));
bi_decl(bi_ptr_global_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_spi_rx_pin, 16));
bi_decl(bi_ptr_global_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_spi_csn_pin, 17));
bi_decl(bi_ptr_global_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_smps_mode_pin, 23));
bi_decl(bi_ptr_global_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_vbus_pin, 24));
bi_decl(bi_ptr_global_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_vsys_pin, 29));

#endif