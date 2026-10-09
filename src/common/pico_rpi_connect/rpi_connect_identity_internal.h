/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RPI_CONNECT_IDENTITY_INTERNAL_H
#define _RPI_CONNECT_IDENTITY_INTERNAL_H

#include "pico/rpi_connect_identity.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RPI_CONNECT_IDENTITY_EXCHANGE_ENDPOINT "/client/device-identity-exchange"

#if !PICO_NONSECURE
// Check the inputs of a device identity exchange request are valid, so the body can be built
int rpi_connect_identity_exchange_validate(const rpi_connect_identity_exchange_t *exchange);

// Build and sign a request which has already been validated with rpi_connect_identity_exchange_validate()
int rpi_connect_identity_exchange_sign_validated(rpi_connect_identity_exchange_t *exchange);
#endif

#ifdef __cplusplus
}
#endif

#endif
