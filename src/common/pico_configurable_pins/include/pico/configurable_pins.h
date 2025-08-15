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
#define PICO_DEFAULT_LED_PIN_INVERTED pico_default_led_pin_inverted
extern const int pico_default_led_pin_inverted;
#define PICO_DEFAULT_WS2812_PIN pico_default_ws2812_pin
extern const int pico_default_ws2812_pin;
#define PICO_DEFAULT_WS2812_POWER_PIN pico_default_ws2812_power_pin
extern const int pico_default_ws2812_power_pin;
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

#define CYW43_DEFAULT_PIN_WL_REG_ON cyw43_default_pin_wl_reg_on
extern const int cyw43_default_pin_wl_reg_on;
#define CYW43_DEFAULT_PIN_WL_DATA_OUT cyw43_default_pin_wl_data_out
extern const int cyw43_default_pin_wl_data_out;
#define CYW43_DEFAULT_PIN_WL_DATA_IN cyw43_default_pin_wl_data_in
extern const int cyw43_default_pin_wl_data_in;
#define CYW43_DEFAULT_PIN_WL_HOST_WAKE cyw43_default_pin_wl_host_wake
extern const int cyw43_default_pin_wl_host_wake;
#define CYW43_DEFAULT_PIN_WL_CLOCK cyw43_default_pin_wl_clock
extern const int cyw43_default_pin_wl_clock;
#define CYW43_DEFAULT_PIN_WL_CS cyw43_default_pin_wl_cs
extern const int cyw43_default_pin_wl_cs;

#define CYW43_WL_GPIO_LED_PIN cyw43_wl_gpio_led_pin
extern const int cyw43_wl_gpio_led_pin;
#define CYW43_WL_GPIO_SMPS_PIN cyw43_wl_gpio_smps_pin
extern const int cyw43_wl_gpio_smps_pin;
#define CYW43_WL_GPIO_VBUS_PIN cyw43_wl_gpio_vbus_pin
extern const int cyw43_wl_gpio_vbus_pin;
#define CYW43_WL_GPIO_VSYS_PIN cyw43_wl_gpio_vsys_pin
extern const int cyw43_wl_gpio_vsys_pin;

#define PICO_COLORED_STATUS_LED_USES_WRGB pico_colored_status_led_uses_wrgb
extern const int pico_colored_status_led_uses_wrgb;

#endif

#endif