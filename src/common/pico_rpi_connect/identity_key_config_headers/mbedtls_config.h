/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Minimal Mbed TLS configuration for pico_rpi_connect_identity_key on its own, e.g. in a TrustZone
// Secure binary: P-256 ECDSA signing and public key PEM writing only, with a CTR_DRBG seeded from the
// hardware entropy source.

#ifndef MBEDTLS_CONFIG_RPI_CONNECT_IDENTITY_KEY_H
#define MBEDTLS_CONFIG_RPI_CONNECT_IDENTITY_KEY_H

/* Workaround for some mbedtls source files using INT_MAX without including limits.h */
#include <limits.h>

#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT

#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECP_NIST_OPTIM
// Smaller precomputed tables, at the cost of some speed
#define MBEDTLS_ECP_WINDOW_SIZE        2
#define MBEDTLS_ECP_FIXED_POINT_OPTIM  0
#define MBEDTLS_AES_FEWER_TABLES
// The CTR_DRBG is only seeded with entropy and a nonce (48 bytes), and the default maximum seed input
// (384 bytes) sizes buffers on the stack, which may be small (e.g. the Secure stack in a TrustZone build)
#define MBEDTLS_CTR_DRBG_MAX_SEED_INPUT 128

#define MBEDTLS_AES_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_BASE64_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_MD_C
#define MBEDTLS_OID_C
#define MBEDTLS_PEM_WRITE_C
#define MBEDTLS_PK_C
#define MBEDTLS_PK_WRITE_C
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_SHA256_C

#endif
