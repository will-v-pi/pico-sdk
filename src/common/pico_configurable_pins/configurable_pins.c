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

bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_uart, 0));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_uart_tx_pin, -1));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_uart_rx_pin, -1));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_led_pin, -1));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_led_pin_inverted, 0));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_ws2812_pin, -1));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_ws2812_power_pin, -1));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_i2c, 0));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_i2c_sda_pin, -1));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_i2c_scl_pin, -1));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_spi, 0));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_spi_sck_pin, -1));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_spi_tx_pin, -1));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_spi_rx_pin, -1));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_default_spi_csn_pin, -1));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_smps_mode_pin, -1));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_vbus_pin, -1));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_vsys_pin, -1));

bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, cyw43_default_pin_wl_reg_on, -1));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, cyw43_default_pin_wl_data_out, -1));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, cyw43_default_pin_wl_data_in, -1));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, cyw43_default_pin_wl_host_wake, -1));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, cyw43_default_pin_wl_clock, -1));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, cyw43_default_pin_wl_cs, -1));

bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, cyw43_wl_gpio_led_pin, -1));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, cyw43_wl_gpio_smps_pin, -1));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, cyw43_wl_gpio_vbus_pin, -1));
bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, cyw43_wl_gpio_vsys_pin, -1));

bi_decl(bi_ptr_const_int32(BINARY_INFO_TAG_CONFIGURABLE_PINS, BINARY_INFO_ID_CONFIGURABLE_PINS, pico_colored_status_led_uses_wrgb, 0));

#endif