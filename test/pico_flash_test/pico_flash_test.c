/**
 * Copyright (c) 2026 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include "hardware/flash.h"
#include "pico/flash.h"
#include "pico/test.h"
#include "pico/stdio.h"

PICOTEST_MODULE_NAME("FLASH", "flash test");

#if LIB_PICO_MULTICORE
#include "pico/multicore.h"
#define FLAG_VALUE 123

void core1_entry() {

    multicore_fifo_push_blocking(FLAG_VALUE);

    flash_safe_execute_core_init();

    uint32_t g = multicore_fifo_pop_blocking();

    PICOTEST_CHECK(g == FLAG_VALUE, "incorrect flag value on core 1");

    while (1)
        printf("Core 1 running\n");
        sleep_ms(10);
}
#endif

#define FLASH_TARGET_OFFSET (256 * 1024)
const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);

static void call_flash_range_erase(void *param) {
    uint32_t offset = (uint32_t)param;
    flash_range_erase(offset, FLASH_SECTOR_SIZE);
}

static void call_flash_range_program(void *param) {
    uint32_t offset = ((uintptr_t*)param)[0];
    const uint8_t *data = (const uint8_t *)((uintptr_t*)param)[1];
    flash_range_program(offset, data, FLASH_PAGE_SIZE);
}

int main() {
    stdio_init_all();

    PICOTEST_START();

#if LIB_PICO_MULTICORE
    PICOTEST_START_SECTION("multicore_init");
        multicore_launch_core1(core1_entry);
        uint32_t g = multicore_fifo_pop_blocking();
        PICOTEST_CHECK_AND_ABORT(g == FLAG_VALUE, "incorrect flag value on core 0");
        multicore_fifo_push_blocking(FLAG_VALUE);
    PICOTEST_END_SECTION();
#endif

    PICOTEST_START_SECTION("flash_ops");
        int rc = flash_safe_execute(call_flash_range_erase, (void*)FLASH_TARGET_OFFSET, 1000);
        PICOTEST_CHECK_AND_ABORT(rc == PICO_OK, "flash erase returned an error")
        for (uint i = 0; i < FLASH_PAGE_SIZE; ++i) {
            PICOTEST_CHECK(flash_target_contents[i] == 0xff, "flash erase did not erase everything")
        }
        uint8_t random_data[FLASH_PAGE_SIZE];
        for (uint i = 0; i < FLASH_PAGE_SIZE; ++i)
            random_data[i] = rand() >> 16;
        uintptr_t params[] = { FLASH_TARGET_OFFSET, (uintptr_t)random_data};
        rc = flash_safe_execute(call_flash_range_program, params, 1000);
        PICOTEST_CHECK_AND_ABORT(rc == PICO_OK, "flash program returned an error")
        for (uint i = 0; i < FLASH_PAGE_SIZE; ++i) {
            PICOTEST_CHECK(flash_target_contents[i] == random_data[i], "flash program incorrect data")
        }
    PICOTEST_END_SECTION();

    PICOTEST_END_TEST();
}
