/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>

#include "pico/bootrom.h"
#include "boot/picoboot.h"
#include "boot/picobin.h"
#if !PICO_RP2040
#include "hardware/rcp.h"
#include "hardware/structs/resets.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/usb_dpram.h"
#endif

/// \tag::table_lookup[]

void *rom_func_lookup(uint32_t code) {
    return rom_func_lookup_inline(code);
}

void *rom_data_lookup(uint32_t code) {
    return rom_data_lookup_inline(code);
}
/// \end::table_lookup[]

bool rom_funcs_lookup(uint32_t *table, unsigned int count) {
    bool ok = true;
    for (unsigned int i = 0; i < count; i++) {
        table[i] = (uintptr_t) rom_func_lookup(table[i]);
        if (!table[i]) ok = false;
    }
    return ok;
}

// The activity LED on RP2350 A2 QFN60 chips doesn't work in Arm mode, so boot into RISC-V if the user
// really, really wants the activity LED
#if PICO_RP2350_A2_SUPPORTED && PICO_RP2350A && !PICO_RISCV && PICO_BOOTROM_WORKAROUND_RP2350_A2_ACTIVITY_LED_BUG
#define rom_reboot_workaround(flags, delay_ms, p0, p1) ({ \
    if (((p0) & BOOTSEL_FLAG_GPIO_PIN_SPECIFIED) && rp2350_rom_version() == 2) \
        rom_reboot((flags) | REBOOT2_FLAG_REBOOT_TO_RISCV, delay_ms, p0, p1); \
    else \
        rom_reboot(flags, delay_ms, p0, p1); \
})
#else
#define rom_reboot_workaround(flags, delay_ms, p0, p1) rom_reboot(flags, delay_ms, p0, p1)
#endif


void __attribute__((noreturn)) rom_reset_usb_boot(uint32_t usb_activity_gpio_pin_mask, uint32_t disable_interface_mask) {
#ifdef ROM_FUNC_RESET_USB_BOOT
    rom_reset_usb_boot_fn func = (rom_reset_usb_boot_fn) rom_func_lookup(ROM_FUNC_RESET_USB_BOOT);
    func(usb_activity_gpio_pin_mask, disable_interface_mask);
#elif defined(ROM_FUNC_REBOOT)
    uint32_t flags = disable_interface_mask;
    if (usb_activity_gpio_pin_mask) {
        flags |= BOOTSEL_FLAG_GPIO_PIN_SPECIFIED;
        // the parameter is actually the gpio number, but we only care if BOOTSEL_FLAG_GPIO_PIN_SPECIFIED
        usb_activity_gpio_pin_mask = (uint32_t)__builtin_ctz(usb_activity_gpio_pin_mask);
    }
    rom_reboot_workaround(REBOOT2_FLAG_REBOOT_TYPE_BOOTSEL | REBOOT2_FLAG_NO_RETURN_ON_SUCCESS, 10, flags, usb_activity_gpio_pin_mask);
    __builtin_unreachable();
#else
    panic_unsupported();
#endif
}

void __attribute__((noreturn)) rom_reset_usb_boot_extra(int usb_activity_gpio_pin, uint32_t disable_interface_mask, bool usb_activity_gpio_pin_active_low) {
#ifdef ROM_FUNC_RESET_USB_BOOT
    (void)usb_activity_gpio_pin_active_low;
    rom_reset_usb_boot_fn func = (rom_reset_usb_boot_fn) rom_func_lookup(ROM_FUNC_RESET_USB_BOOT);
    func(usb_activity_gpio_pin < 0 ? 0 : (1u << usb_activity_gpio_pin), disable_interface_mask);
#elif defined(ROM_FUNC_REBOOT)
    uint32_t flags = disable_interface_mask;
    if (usb_activity_gpio_pin >= 0) {
        flags |= BOOTSEL_FLAG_GPIO_PIN_SPECIFIED;
        if (usb_activity_gpio_pin_active_low) {
            flags |= BOOTSEL_FLAG_GPIO_PIN_ACTIVE_LOW;
        }
    }
    rom_reboot_workaround(REBOOT2_FLAG_REBOOT_TYPE_BOOTSEL | REBOOT2_FLAG_NO_RETURN_ON_SUCCESS, 10, flags, (uint)usb_activity_gpio_pin);
    __builtin_unreachable();
#else
    panic_unsupported();
#endif
}

#if !PICO_RP2040
bool rom_get_boot_random(uint32_t out[4]) {
    uint32_t result[5];
    rom_get_sys_info_fn func = (rom_get_sys_info_fn) rom_func_lookup_inline(ROM_FUNC_GET_SYS_INFO);
    if (5 == func(result, count_of(result), SYS_INFO_BOOT_RANDOM)) {
        for(uint i=0;i<4;i++) {
            out[i] = result[i+1];
        }
        return true;
    }
    return false;
}

#if !PICO_RISCV
#pragma GCC diagnostic push
// diagnostic: GCC thinks near-zero value is a null pointer member access, but it's not
#pragma GCC diagnostic ignored "-Warray-bounds"
void __attribute__((noreturn)) rom_chain_image_no_return(uint32_t region_base, uint32_t region_size) {
    rom_chain_image_fn func = (rom_chain_image_fn) rom_func_lookup_inline(ROM_FUNC_CHAIN_IMAGE);
    // Entry 2 of the bootrom vector table is the NMI handler (with the Thumb bit already set)
    uintptr_t nmi_handler = *(const uintptr_t *)(BOOTROM_VTABLE_OFFSET + 2 * sizeof(uintptr_t));
    bootrom_release_lock(BOOTROM_LOCK_ENABLE);
    (void)save_and_disable_interrupts();
    // Use the bootrom vector table, as existing vector table and handlers may be overwritten by the chained image
    scb_hw->vtor = BOOTROM_VTABLE_OFFSET;
    __dsb();
    __isb();
    // Use USB RAM for the workarea and stack, as it can't be overwritten by the chained image
    // USB RAM needs the clk_sys side of USBCTRL out of reset, so copy what the bootrom does
    io_rw_32 *clear_resets_reg = hw_clear_alias(&resets_hw->reset);
    *clear_resets_reg = RESETS_RESET_USBCTRL_BITS;
    (void) *clear_resets_reg;
    uintptr_t stack_top = USBCTRL_DPRAM_BASE + USB_DPRAM_SIZE;
    uintptr_t stack_limit = USBCTRL_DPRAM_BASE + PICO_BOOTROM_CHAIN_IMAGE_NO_RETURN_WORKAREA_SIZE;
    register uintptr_t r0 asm("r0") = USBCTRL_DPRAM_BASE;
    register uint32_t r1 asm("r1") = PICO_BOOTROM_CHAIN_IMAGE_NO_RETURN_WORKAREA_SIZE;
    register uint32_t r2 asm("r2") = region_base;
    register uint32_t r3 asm("r3") = region_size;
    // Explicit registers for the remaining operands too, as GCC can't otherwise allocate them at -O0
    // (avoiding r7, which is the frame pointer at -O0)
    register uintptr_t r4 asm("r4") = (uintptr_t)func;
    register uintptr_t r5 asm("r5") = nmi_handler;
    register uintptr_t r6 asm("r6") = stack_top;
    register uintptr_t ip asm("ip") = stack_limit;
    // Switch to the USB RAM stack, then tail call into the bootrom with the return address set to the bootrom NMI
    // handler, as the caller may have been overwritten by the time chain_image returns
    pico_default_asm_volatile(
        "msr msp, %[stack]\n"
        "msr msplim, %[stack_limit]\n"
        "mov lr, %[ret]\n"
        "bx %[func]\n"
        :
        : "r" (r0), "r" (r1), "r" (r2), "r" (r3), [func] "r" (r4), [ret] "r" (r5), [stack] "r" (r6), [stack_limit] "r" (ip)
        : "lr", "memory"
    );
    __builtin_unreachable();
}
#pragma GCC diagnostic pop
#endif

int rom_add_flash_runtime_partition(uint32_t start_offset, uint32_t size, uint32_t permissions) {
    if ((start_offset) & 4095 || (size & 4095)) return PICO_ERROR_BAD_ALIGNMENT;
    if (!size || start_offset + size > 32 * 1024 * 1024) return PICO_ERROR_INVALID_ARG;
    if (permissions & ~PICOBIN_PARTITION_PERMISSIONS_BITS) return PICO_ERROR_INVALID_ARG;

    void **ptr = (void **)rom_data_lookup(ROM_DATA_PARTITION_TABLE_PTR);
    assert(ptr);
    assert(*ptr);
    struct pt {
        struct {
            uint8_t partition_count;
            uint8_t permission_partition_count; // >= partition_count and includes any regions added at runtime
            bool loaded;
        };
        uint32_t unpartitioned_space_permissions_and_flags;
        resident_partition_t partitions[PARTITION_TABLE_MAX_PARTITIONS];
    } *pt = (struct pt *)*ptr;
    assert(pt->loaded); // even if empty it should have been populated by the bootrom
    if (pt->permission_partition_count < pt->partition_count) pt->permission_partition_count = pt->partition_count;
    if (pt->permission_partition_count < PARTITION_TABLE_MAX_PARTITIONS) {
        pt->partitions[pt->permission_partition_count].permissions_and_location = permissions |
                ((start_offset / 4096) << PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_LSB) |
                ((start_offset + size - 4096) / 4096) << PICOBIN_PARTITION_LOCATION_LAST_SECTOR_LSB;
        pt->partitions[pt->permission_partition_count].permissions_and_flags = permissions;
        return pt->permission_partition_count++;
    }
    return PICO_ERROR_INSUFFICIENT_RESOURCES;
}

int rom_pick_ab_partition_during_update(uint32_t *workarea_base, uint32_t workarea_size, uint partition_a_num) {
#if !PICO_RP2040
    // Generated from adding the following code into the bootrom
    // scan_workarea_t* scan_workarea = (scan_workarea_t*)workarea;
    // printf("VERSION_DOWNGRADE_ERASE_ADDR %08x\n", &(always->zero_init.version_downgrade_erase_flash_addr));
    // printf("TBYB_FLAG_ADDR %08x\n", &(always->zero_init.tbyb_flag_flash_addr));
    // printf("IMAGE_DEF_VERIFIED %08x\n", (uint32_t)&(scan_workarea->parsed_block_loops[0].image_def.core.verified) - (uint32_t)scan_workarea);
    // printf("IMAGE_DEF_TBYB_FLAGGED %08x\n", (uint32_t)&(scan_workarea->parsed_block_loops[0].image_def.core.tbyb_flagged) - (uint32_t)scan_workarea);
    // printf("IMAGE_DEF_BASE %08x\n", (uint32_t)&(scan_workarea->parsed_block_loops[0].image_def.core.enclosing_window.base) - (uint32_t)scan_workarea);
    // printf("IMAGE_DEF_REL_BLOCK_OFFSET %08x\n", (uint32_t)&(scan_workarea->parsed_block_loops[0].image_def.core.window_rel_block_offset) - (uint32_t)scan_workarea);
    #define VERSION_DOWNGRADE_ERASE_ADDR *(uint32_t*)0x400e0338
    #define TBYB_FLAG_ADDR *(uint32_t*)0x400e0348
    #define IMAGE_DEF_VERIFIED(scan_workarea) *(uint32_t*)(0x64 + (uint32_t)scan_workarea)
    #define IMAGE_DEF_TBYB_FLAGGED(scan_workarea) *(bool*)(0x4c + (uint32_t)scan_workarea)
    #define IMAGE_DEF_BASE(scan_workarea) *(uint32_t*)(0x54 + (uint32_t)scan_workarea)
    #define IMAGE_DEF_REL_BLOCK_OFFSET(scan_workarea) *(uint32_t*)(0x5c + (uint32_t)scan_workarea)
#else
    // Prevent linting errors
    #define VERSION_DOWNGRADE_ERASE_ADDR *(uint32_t*)NULL
    #define TBYB_FLAG_ADDR *(uint32_t*)NULL
    #define IMAGE_DEF_VERIFIED(scan_workarea) *(uint32_t*)(NULL + (uint32_t)scan_workarea)
    #define IMAGE_DEF_TBYB_FLAGGED(scan_workarea) *(bool*)(NULL + (uint32_t)scan_workarea)
    #define IMAGE_DEF_BASE(scan_workarea) *(uint32_t*)(NULL + (uint32_t)scan_workarea)
    #define IMAGE_DEF_REL_BLOCK_OFFSET(scan_workarea) *(uint32_t*)(NULL + (uint32_t)scan_workarea)

    panic_unsupported();
#endif

    uint32_t flash_update_base = 0;
    bool tbyb_boot = false;
    uint32_t saved_erase_addr = 0;
    if (rom_get_last_boot_type() == BOOT_TYPE_FLASH_UPDATE) {
        // For a flash update boot, get the flash update base
        boot_info_t boot_info = {};
        int ret = rom_get_boot_info(&boot_info);
        if (ret) {
            flash_update_base = boot_info.reboot_params[0];
            if (boot_info.tbyb_and_update_info & BOOT_TBYB_AND_UPDATE_FLAG_BUY_PENDING) {
                // A buy is pending, so the main software has not been bought
                tbyb_boot = true;
                // Save the erase address, as this will be overwritten by rom_pick_ab_partition
                saved_erase_addr = VERSION_DOWNGRADE_ERASE_ADDR;
            }
        }
    }

    int rc = rom_pick_ab_partition((uint8_t*)workarea_base, workarea_size, partition_a_num, flash_update_base);

    if (!rcp_is_true(IMAGE_DEF_VERIFIED(workarea_base))) {
        if (IMAGE_DEF_VERIFIED(workarea_base) == 0) {
            // Bootrom did not perform verification due to no block loop in the B partition
            return BOOTROM_ERROR_INVALID_DATA;
        } else {
            // Chosen partition failed verification
            return BOOTROM_ERROR_NOT_FOUND;
        }
    }

    if (IMAGE_DEF_TBYB_FLAGGED(workarea_base)) {
        // The chosen partition is TBYB
        if (tbyb_boot) {
            // The boot partition is also TBYB - cannot update both, so prioritise boot partition
            // Restore the erase address saved earlier
            VERSION_DOWNGRADE_ERASE_ADDR = saved_erase_addr;
            return BOOTROM_ERROR_NOT_PERMITTED;
        } else {
            // Update the tbyb flash address, so that explicit_buy will clear the flag for the chosen partition
            TBYB_FLAG_ADDR =
                    IMAGE_DEF_BASE(workarea_base)
                    + IMAGE_DEF_REL_BLOCK_OFFSET(workarea_base) + 4;
        }
    } else {
        // The chosen partition is not TBYB
        if (tbyb_boot && saved_erase_addr) {
            // The boot partition was TBYB, and requires an erase
            if (VERSION_DOWNGRADE_ERASE_ADDR) {
                // But both the chosen partition requires an erase too
                // As before, prioritise the boot partition, and restore it's saved erase_address
                VERSION_DOWNGRADE_ERASE_ADDR = saved_erase_addr;
                return BOOTROM_ERROR_NOT_PERMITTED;
            } else {
                // The chosen partition doesn't require an erase, so we're fine
                VERSION_DOWNGRADE_ERASE_ADDR = saved_erase_addr;
            }
        }
    }

    return rc;
}

int rom_get_partition_by_id(uint64_t id) {
    // each partition returns a maximum of 4 words, plus 1 for the supported flags
    uint32_t buffer[(PARTITION_TABLE_MAX_PARTITIONS * 4) + 1];
    const uint32_t flags = PT_INFO_PARTITION_LOCATION_AND_FLAGS | PT_INFO_PARTITION_ID;
    int ret = rom_get_partition_table_info(buffer, count_of(buffer), flags);
    if (ret < 0) return ret;
    if (ret == 0 || buffer[0] != flags) return PICO_ERROR_GENERIC;

    int i = 1;
    int p = 0;
    while (i < ret) {
        i++;
        uint32_t flags_and_permissions = buffer[i++];
        bool has_id = (flags_and_permissions & PICOBIN_PARTITION_FLAGS_HAS_ID_BITS);
        if (has_id) {
            uint64_t partition_id = 0;
            partition_id |= buffer[i++];
            partition_id |= ((uint64_t)(buffer[i++]) << 32ull);
            if (partition_id == id) {
                return p;
            }
        }
        p++;
    }
    return BOOTROM_ERROR_NOT_FOUND;
}

int rom_program_invalid_block_to_b_partition(uint pi_a) {
    int pi_b = rom_get_b_partition(pi_a);
    if (pi_b < 0) return pi_b;

    uint32_t buffer[4 + 1] = {}; // single partition, with maximum of 4 words returned, plus 1
    int ret = rom_get_partition_table_info(buffer, count_of(buffer), PT_INFO_PARTITION_LOCATION_AND_FLAGS | PT_INFO_SINGLE_PARTITION | ((uint32_t)pi_b << 24));
    hard_assert(buffer[0] == (PT_INFO_PARTITION_LOCATION_AND_FLAGS | PT_INFO_SINGLE_PARTITION));
    hard_assert(ret == 3);

    uint32_t location_and_permissions = buffer[1];
    // 4096 byte flash sectors
    uint32_t saddr = ((location_and_permissions >> PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_LSB) & 0x1fffu) * 4096u;

    // one 256 byte flash page
    uint32_t invalid_block[256 / sizeof(uint32_t)] = PICOBIN_INVALID_BLOCK;

    saddr += XIP_BASE;
    cflash_flags_t flags = {
        (CFLASH_OP_VALUE_PROGRAM << CFLASH_OP_LSB)
        | (CFLASH_SECLEVEL_VALUE_SECURE << CFLASH_SECLEVEL_LSB)
        | (CFLASH_ASPACE_VALUE_STORAGE << CFLASH_ASPACE_LSB)
    };
    ret = rom_flash_op(flags, saddr, sizeof(invalid_block), (uint8_t*)invalid_block);
    return ret;
}

int rom_pick_ab_partition_by_id(uint32_t *workarea_base, uint32_t workarea_size, uint64_t id, bool ensure_verification) {
    int picked_p = rom_get_partition_by_id(id);
    if (picked_p < 0) return picked_p;

#ifdef __riscv
    // Increased bootrom stack is required for this function
    bootrom_stack_t stack = {
        .base = malloc(0x400),
        .size = 0x400
    };
    if (!stack.base) return PICO_ERROR_INSUFFICIENT_RESOURCES;
    rom_set_bootrom_stack(&stack);
#endif
    int check_p = rom_pick_ab_partition_during_update(workarea_base, workarea_size, (uint)picked_p);
    if (check_p == BOOTROM_ERROR_INVALID_DATA) {
        // No block loop in the B partition, so the A partition has not been verified
        if (ensure_verification) {
            // Write an invalid block to the B partition, in order to verify the A partition
            int rc = rom_program_invalid_block_to_b_partition((uint)picked_p);
            if (rc) {
                picked_p = rc < 0 ? rc : PICO_ERROR_GENERIC;
            } else {
                picked_p = rom_pick_ab_partition_during_update(workarea_base, workarea_size, (uint)picked_p);
            }
        }
        // otherwise allow the unverified A partition to be used
    } else {
        picked_p = check_p;
    }
#ifdef __riscv
    // Reset bootrom stack
    rom_set_bootrom_stack(&stack);
    free(stack.base);
#endif
    return picked_p;
}
#endif