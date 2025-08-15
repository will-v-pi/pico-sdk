/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PICO_CONFIGURABLE_PINS_H
#define _PICO_CONFIGURABLE_PINS_H

#include "pico.h"

#if PICO_SUPPORT_CONFIGURABLE_PINS

#define PICO_DEFAULT_UART pico_default_uart
extern const int pico_default_uart;
#define PICO_DEFAULT_UART_TX_PIN pico_default_uart_tx_pin
extern const int pico_default_uart_tx_pin;
#define PICO_DEFAULT_UART_RX_PIN pico_default_uart_rx_pin
extern const int pico_default_uart_rx_pin;
#define PICO_DEFAULT_LED_PIN pico_default_led_pin
extern const int pico_default_led_pin;
#define PICO_DEFAULT_I2C pico_default_i2c
extern const int pico_default_i2c;
#define PICO_DEFAULT_I2C_SDA_PIN pico_default_i2c_sda_pin
extern const int pico_default_i2c_sda_pin;
#define PICO_DEFAULT_I2C_SCL_PIN pico_default_i2c_scl_pin
extern const int pico_default_i2c_scl_pin;
#define PICO_DEFAULT_SPI pico_default_spi
extern const int pico_default_spi;
#define PICO_DEFAULT_SPI_SCK_PIN pico_default_spi_sck_pin
extern const int pico_default_spi_sck_pin;
#define PICO_DEFAULT_SPI_TX_PIN pico_default_spi_tx_pin
extern const int pico_default_spi_tx_pin;
#define PICO_DEFAULT_SPI_RX_PIN pico_default_spi_rx_pin
extern const int pico_default_spi_rx_pin;
#define PICO_DEFAULT_SPI_CSN_PIN pico_default_spi_csn_pin
extern const int pico_default_spi_csn_pin;
#define PICO_SMPS_MODE_PIN pico_smps_mode_pin
extern const int pico_smps_mode_pin;
#define PICO_VBUS_PIN pico_vbus_pin
extern const int pico_vbus_pin;
#define PICO_VSYS_PIN pico_vsys_pin
extern const int pico_vsys_pin;

#endif

#endif