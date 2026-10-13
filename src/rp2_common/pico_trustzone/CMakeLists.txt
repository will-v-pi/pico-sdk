if (NOT TARGET pico_trustzone)
    pico_add_library(pico_trustzone)
    target_include_directories(pico_trustzone_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

    target_sources(pico_trustzone INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/trustzone.c)

    pico_mirrored_target_link_libraries(pico_trustzone INTERFACE
            hardware_exception
            hardware_flash
            pico_rand
            hardware_dma
            hardware_irq
            pico_stdio
            hardware_pio
            pico_bootrom)


    # pico_set_security_options(SECURE_TARGET NONSECURE_TARGET <OPTIONS>...)
    # \brief_nodesc\ Set matching security options for a secure and non-secure target
    #
    # Set matching security options for a secure and non-secure target, so they have a compatible set of features.
    #
    # Also sets PICO_SECURE=1 and PICO_NONSECURE=1 on the secure and non-secure targets respectively, along with
    # any other required defines (eg PICO_USE_STACK_GUARDS=1 on the secure target).
    #
    # The options are:
    # - STDIO: Allow non-secure to use secure stdio
    # - RAND: Allow non-secure to get random numbers
    # - DMA: Allow non-secure to request DMA channels
    # - USER_IRQ: Allow non-secure to request user IRQs
    # - PIO: Allow non-secure to request PIOs
    # - GPIO: Allow non-secure to access GPIOs assigned to non-secure (eg with gpio_assign_to_ns)
    # - USB: Allow non-secure to access USB
    # - RESETS: Allow non-secure to access resets specified by PICO_ALLOW_NONSECURE_RESETS_MASK (automatically set if USB is set)
    # - BOOTROM_LOCKING: Allow non-secure to access bootrom locks less than PICO_ALLOW_NONSECURE_BOOTROM_LOCK_MAX (defaults to allow all)
    # - PICK_AB: Allow non-secure to call rom_pick_ab_partition_during_update
    # - ROLL_PARTITION: Allow non-secure to call flash_roll_qmi_add_partition_at_window
    # - BUY: Allow non-secure to call rom_explicit_buy
    # - UF2_TARGET_PARTITION: Allow non-secure to call rom_get_uf2_target_partition
    # - NONSECURE_TIMER <index>: Assign specified timer to non-secure
    #
    # \param\ SECURE_TARGET The secure target
    # \param\ NONSECURE_TARGET The non-secure target
    # \param\ OPTIONS The options to set
    function(pico_set_security_options SECURE_TARGET NONSECURE_TARGET)
        set(options STDIO RAND DMA USER_IRQ PIO GPIO USB RESETS BOOTROM_LOCKING PICK_AB ROLL_PARTITION BUY UF2_TARGET_PARTITION)
        set(oneValueArgs NONSECURE_TIMER)
        cmake_parse_arguments(PARSE_ARGV 2 OPTS "${options}" "${oneValueArgs}" "")

        target_compile_definitions(${SECURE_TARGET} PRIVATE
            PICO_SECURE=1
    
            # Stack guards are required
            PICO_USE_STACK_GUARDS=1
        )
    
        target_compile_definitions(${NONSECURE_TARGET} PRIVATE
            PICO_NONSECURE=1
        )

        # Options that require resets
        if ((NOT OPTS_RESETS) AND OPTS_USB)
            set(OPTS_RESETS 1)
        endif()
    
        foreach(arg IN LISTS options)
            if (OPTS_${arg})
                target_compile_definitions(${SECURE_TARGET} PRIVATE PICO_ALLOW_NONSECURE_${arg}=1)
                target_compile_definitions(${NONSECURE_TARGET} PRIVATE PICO_ALLOW_NONSECURE_${arg}=1)
            endif()
        endforeach()

        if (OPTS_NONSECURE_TIMER)
            target_compile_definitions(${SECURE_TARGET} PRIVATE PICO_ASSIGN_NONSECURE_TIMER=${OPTS_NONSECURE_TIMER})
            target_compile_definitions(${NONSECURE_TARGET} PRIVATE PICO_DEFAULT_TIMER=${OPTS_NONSECURE_TIMER})
        endif()
    endfunction()

    # pico_set_security_ram_split(SECURE_TARGET NONSECURE_TARGET <OPTIONS>...)
    # \brief_nodesc\ Set ram split for a secure and non-secure target
    #
    # Set ram split for a secure and non-secure target, so they don't use the same memory.
    #
    # Each split type option requires arguments to specify the memory sizes. You can only select
    # one split type
    #
    # The split types available are:
    # - SIMPLE <SECURE_LENGTH> <SECURE_SCRATCH_LENGTH>: Secure using start of main SRAM, NonSecure using end of main SRAM and scratch
    # - SCRATCH_EACH <SECURE_LENGTH>: Secure using start of main SRAM plus scratch X as stack, NonSecure using end of main SRAM plus scratch Y as stack
    # - SECURE_SCRATCH <SECURE_LENGTH> <NONSECURE_SCRATCH_LENGTH>: Secure using start of main SRAM plus all of scratch, NonSecure using end of main SRAM
    #
    # Additional options are:
    # - NO_FLASH: Assumes NS VTOR is at start of it's SRAM region, rather than at the start of flash. The
    #   NonSecure binary can then be a no_flash binary, embedded in the Secure binary with pico_embed_nonsecure_binary()
    #
    # \param\ SECURE_TARGET The secure target
    # \param\ NONSECURE_TARGET The non-secure target
    # \param\ OPTIONS The options to set
    function(pico_set_security_ram_split SECURE_TARGET NONSECURE_TARGET)
        set(options NO_FLASH)
        set(multiValueArgs SIMPLE SCRATCH_EACH SECURE_SCRATCH)
        cmake_parse_arguments(PARSE_ARGV 2 OPTS "${options}" "" "${multiValueArgs}")

        set(HAS_SPLIT_TYPE FALSE)
        foreach(arg IN LISTS multiValueArgs)
            if (OPTS_${arg})
                if (HAS_SPLIT_TYPE)
                    message(FATAL_ERROR "Multiple split types passed to pico_set_security_ram_split")
                endif()
                set(HAS_SPLIT_TYPE TRUE)
            endif()
        endforeach()

        if (NOT HAS_SPLIT_TYPE)
            message(FATAL_ERROR "No split type passed to pico_set_security_ram_split")
        endif()

        if (OPTS_SIMPLE)
            list(GET OPTS_SIMPLE 0 SECURE_LENGTH)
            list(GET OPTS_SIMPLE 1 SECURE_SCRATCH_LENGTH)

            pico_set_linker_script_var(${SECURE_TARGET} RAM_ORIGIN "RAM_ORIGIN_DEFAULT")
            pico_set_linker_script_var(${SECURE_TARGET} RAM_LENGTH "${SECURE_LENGTH}-(${SECURE_SCRATCH_LENGTH}*2)")
            pico_set_linker_script_var(${SECURE_TARGET} SCRATCH_X_ORIGIN "RAM_ORIGIN+RAM_LENGTH")
            pico_set_linker_script_var(${SECURE_TARGET} SCRATCH_X_LENGTH ${SECURE_SCRATCH_LENGTH})
            pico_set_linker_script_var(${SECURE_TARGET} SCRATCH_Y_ORIGIN "SCRATCH_X_ORIGIN+SCRATCH_X_LENGTH")
            pico_set_linker_script_var(${SECURE_TARGET} SCRATCH_Y_LENGTH SCRATCH_X_LENGTH)

            pico_set_linker_script_var(${NONSECURE_TARGET} RAM_ORIGIN "RAM_ORIGIN_DEFAULT+${SECURE_LENGTH}")
            pico_set_linker_script_var(${NONSECURE_TARGET} RAM_LENGTH "RAM_LENGTH_DEFAULT-${SECURE_LENGTH}")

            target_compile_definitions(${SECURE_TARGET} PRIVATE PICO_SECURITY_SPLIT_SIMPLE=1)
        elseif(OPTS_SCRATCH_EACH)
            list(GET OPTS_SCRATCH_EACH 0 SECURE_LENGTH)

            pico_set_linker_script_var(${SECURE_TARGET} RAM_ORIGIN "RAM_ORIGIN_DEFAULT")
            pico_set_linker_script_var(${SECURE_TARGET} RAM_LENGTH "${SECURE_LENGTH}-SCRATCH_X_LENGTH_DEFAULT")
            pico_set_linker_script_var(${SECURE_TARGET} SCRATCH_X_ORIGIN "RAM_ORIGIN+RAM_LENGTH")
            pico_set_linker_script_var(${SECURE_TARGET} SCRATCH_Y_ORIGIN "SCRATCH_X_ORIGIN_DEFAULT")

            pico_set_linker_script_var(${NONSECURE_TARGET} RAM_ORIGIN "RAM_ORIGIN_DEFAULT+${SECURE_LENGTH}")
            pico_set_linker_script_var(${NONSECURE_TARGET} RAM_LENGTH "RAM_LENGTH_DEFAULT-${SECURE_LENGTH}-SCRATCH_X_LENGTH_DEFAULT")
            pico_set_linker_script_var(${NONSECURE_TARGET} SCRATCH_X_ORIGIN "RAM_ORIGIN+RAM_LENGTH")

            target_compile_definitions(${SECURE_TARGET} PRIVATE PICO_SECURITY_SPLIT_SCRATCH_EACH=1)
        elseif(OPTS_SECURE_SCRATCH)
            list(GET OPTS_SECURE_SCRATCH 0 SECURE_LENGTH)
            list(GET OPTS_SECURE_SCRATCH 1 NONSECURE_SCRATCH_LENGTH)

            pico_set_linker_script_var(${SECURE_TARGET} RAM_LENGTH "${SECURE_LENGTH}")

            pico_set_linker_script_var(${NONSECURE_TARGET} RAM_ORIGIN "RAM_ORIGIN_DEFAULT+${SECURE_LENGTH}")
            pico_set_linker_script_var(${NONSECURE_TARGET} RAM_LENGTH "RAM_LENGTH_DEFAULT-${SECURE_LENGTH}-(${NONSECURE_SCRATCH_LENGTH}*2)")
            pico_set_linker_script_var(${NONSECURE_TARGET} SCRATCH_X_ORIGIN "RAM_ORIGIN+RAM_LENGTH")
            pico_set_linker_script_var(${NONSECURE_TARGET} SCRATCH_X_LENGTH ${NONSECURE_SCRATCH_LENGTH})
            pico_set_linker_script_var(${NONSECURE_TARGET} SCRATCH_Y_ORIGIN "SCRATCH_X_ORIGIN+SCRATCH_X_LENGTH")
            pico_set_linker_script_var(${NONSECURE_TARGET} SCRATCH_Y_LENGTH SCRATCH_X_LENGTH)

            target_compile_definitions(${SECURE_TARGET} PRIVATE PICO_SECURITY_SPLIT_SECURE_SCRATCH=1)
        endif()

        target_compile_definitions(${SECURE_TARGET} PRIVATE PICO_SECURITY_SPLIT_CONFIGURED=1)

        if (OPTS_NO_FLASH)
            target_compile_definitions(${SECURE_TARGET} PRIVATE PICO_SECURITY_SPLIT_NO_FLASH=1)
            # Start of the NonSecure SRAM region, where pico_embed_nonsecure_binary() places the NonSecure binary
            pico_set_linker_script_var(${SECURE_TARGET} NONSECURE_RAM_ORIGIN "RAM_ORIGIN_DEFAULT+${SECURE_LENGTH}")
        else()
            target_compile_definitions(${SECURE_TARGET} PRIVATE PICO_SECURITY_SPLIT_NO_FLASH=0)
        endif()

    endfunction()

    # pico_embed_nonsecure_binary(SECURE_TARGET NONSECURE_TARGET)
    # \brief_nodesc\ Embed a no_flash NonSecure binary in a Secure binary
    #
    # Embeds the NonSecure binary in the Secure binary, at the start of the NonSecure SRAM region,
    # rather than running the NonSecure binary from flash. The NonSecure binary is then loaded along with the
    # Secure binary (e.g. by the bootrom, for a packaged Secure binary), is covered by any signature or hash of
    # the Secure binary, and can be launched with secure_launch_nonsecure_binary_default().
    #
    # The NonSecure binary must be a no_flash binary, and pico_set_security_ram_split() must be called with the
    # NO_FLASH option.
    #
    # The NonSecure binary isn't booted by the bootrom, or visible to picotool, so this removes its binary info and
    # metadata block, as all of that should be set on the Secure binary instead.
    #
    # \param\ SECURE_TARGET The secure target
    # \param\ NONSECURE_TARGET The non-secure target, which must be a no_flash binary
    function(pico_embed_nonsecure_binary SECURE_TARGET NONSECURE_TARGET)
        set(NONSECURE_BIN ${CMAKE_CURRENT_BINARY_DIR}/${SECURE_TARGET}_nonsecure_image.bin)
        set(NONSECURE_ASM ${CMAKE_CURRENT_BINARY_DIR}/${SECURE_TARGET}_nonsecure_image.S)
        set(NONSECURE_LD ${CMAKE_CURRENT_BINARY_DIR}/${SECURE_TARGET}_nonsecure_image.ld)

        # Binary info and the metadata block aren't used in an embedded binary
        target_compile_definitions(${NONSECURE_TARGET} PRIVATE
            PICO_NO_BINARY_INFO=1
            PICO_CRT0_INCLUDE_PICOBIN_BLOCK=0
        )

        add_custom_command(OUTPUT ${NONSECURE_BIN}
            COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${NONSECURE_TARGET}> ${NONSECURE_BIN}
            DEPENDS ${NONSECURE_TARGET}
            VERBATIM)

        file(GENERATE OUTPUT ${NONSECURE_ASM} CONTENT
            ".section .nonsecure_image, \"a\"\n.incbin \"${NONSECURE_BIN}\"\n")
        target_sources(${SECURE_TARGET} PRIVATE ${NONSECURE_ASM})
        set_source_files_properties(${NONSECURE_ASM} PROPERTIES OBJECT_DEPENDS ${NONSECURE_BIN})

        # Place it at the start of the NonSecure SRAM region (NONSECURE_RAM_ORIGIN is set by
        # pico_set_security_ram_split() with NO_FLASH)
        file(GENERATE OUTPUT ${NONSECURE_LD} CONTENT
"SECTIONS
{
    .nonsecure_image NONSECURE_RAM_ORIGIN : {
        KEEP(*(.nonsecure_image))
    }
}
INSERT AFTER .stack_dummy;
")
        target_link_options(${SECURE_TARGET} PRIVATE "LINKER:-T,${NONSECURE_LD}")
        set_property(TARGET ${SECURE_TARGET} APPEND PROPERTY LINK_DEPENDS ${NONSECURE_LD})
    endfunction()
endif()
