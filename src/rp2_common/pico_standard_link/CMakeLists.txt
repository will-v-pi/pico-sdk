if (NOT TARGET pico_standard_link)
    pico_add_library(pico_standard_link)

    if (TARGET boot_stage2_headers)
        target_link_libraries(pico_standard_link INTERFACE boot_stage2_headers)
    endif()

    pico_register_common_scope_var(PICO_LINKER_SCRIPT_INCLUDE_DIRS)

    # add the default linker script include directories
    list(APPEND PICO_LINKER_SCRIPT_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/script_include)

    # pico_add_link_depend(TARGET dependency)
    # \brief\ Add a link time dependency to the target
    #
    # \param\ dependency The dependency to add
    function(pico_add_link_depend TARGET dependency)
        get_target_property(target_type ${TARGET} TYPE)
        if (${target_type} STREQUAL "INTERFACE_LIBRARY")
            set(PROP "INTERFACE_LINK_DEPENDS")
        else()
            set(PROP "LINK_DEPENDS")
        endif()
        get_target_property(_LINK_DEPENDS ${TARGET} ${PROP})
        if (NOT _LINK_DEPENDS)
            set(_LINK_DEPENDS ${dependency})
        else()
            list(APPEND _LINK_DEPENDS ${dependency})
        endif()
        set_target_properties(${TARGET} PROPERTIES ${PROP} "${_LINK_DEPENDS}")
    endfunction()

    # pico_add_memmap_link_depends(TARGET TYPE)
    # \brief\ Add link dependencies for the binary type to the target
    #
    # \param\ TYPE The binary type to use
    function(pico_add_memmap_link_depends TARGET TYPE)
        include(${PICO_LINKER_SCRIPT_PATH}/memmap_${TYPE}.cmake)
        pico_add_memmap_link_depends_internal(${TARGET})
    endfunction()

    # pico_check_linker_script(LDSCRIPT)
    # \brief_nodesc\ Check the linker script for compatibility
    #
    # Checks the linker script for compatibility with the current SDK version,
    # and if not, raises warnings and enables workarounds to maintain
    # compatibility where possible.
    #
    # \param\ LDSCRIPT Full path to the linker script to check
    function(pico_check_linker_script TARGET LDSCRIPT)
        if (EXISTS ${LDSCRIPT})
            file(READ ${LDSCRIPT} LDSCRIPT_CONTENTS)
        else()
            return()
        endif()

        # Check if the linker script uses KEEP to keep the .stack and .heap sections
        # and if not, set PICO_CRT0_ALLOCATE_SPACERS to 0 to maintain compatibility
        string(FIND "${LDSCRIPT_CONTENTS}" "KEEP(*(.stack*))" KEEP_STACK_FOUND)
        string(FIND "${LDSCRIPT_CONTENTS}" "KEEP(*(.heap*))" KEEP_HEAP_FOUND)
        string(FIND "${LDSCRIPT_CONTENTS}" "*(.stack*)" STACK_FOUND)
        string(FIND "${LDSCRIPT_CONTENTS}" "*(.heap*)" HEAP_FOUND)
        set(PICO_CRT0_ALLOCATE_SPACERS TRUE)
        if ((${STACK_FOUND} GREATER -1) AND NOT (${KEEP_STACK_FOUND} GREATER -1))
            message(WARNING "Linker script ${LDSCRIPT} does not KEEP the .stack section - replace `*(.stack*)` with `KEEP(*(.stack*))`")
            set(PICO_CRT0_ALLOCATE_SPACERS FALSE)
        endif()
        if ((${HEAP_FOUND} GREATER -1) AND NOT (${KEEP_HEAP_FOUND} GREATER -1))
            message(WARNING "Linker script ${LDSCRIPT} does not KEEP the .heap section - replace `*(.heap*)` with `KEEP(*(.heap*))`")
            set(PICO_CRT0_ALLOCATE_SPACERS FALSE)
        endif()
        if (NOT ${PICO_CRT0_ALLOCATE_SPACERS})
            message(WARNING "Linker script ${LDSCRIPT} is incompatible with certain Pico SDK >2.1.1 features; setting PICO_CRT0_ALLOCATE_SPACERS=0 as a workaround")
            target_compile_definitions(${TARGET} PRIVATE PICO_CRT0_ALLOCATE_SPACERS=0)
        endif()
    endfunction()

    # pico_set_linker_script(TARGET LDSCRIPT)
    # \brief\ Set the linker script for the target
    #
    # \param\ LDSCRIPT Full path to the linker script to set
    function(pico_set_linker_script TARGET LDSCRIPT)
        pico_check_linker_script(${TARGET} ${LDSCRIPT})
        set_target_properties(${TARGET} PROPERTIES PICO_TARGET_LINKER_SCRIPT ${LDSCRIPT})
    endfunction()

    # pico_set_linker_script_var(TARGET NAME VALUE)
    # \brief\ Set the linker script for the target
    #
    # \param\ NAME Name of variable to set
    # \param\ VALUE Value of variable to set
    function(pico_set_linker_script_var TARGET NAME VALUE)
        set_property(TARGET ${TARGET} APPEND PROPERTY PICO_TARGET_LINKER_SCRIPT_VARS "${NAME}=${VALUE}")
    endfunction()

    # pico_add_linker_script_override_path(TARGET PATH [FILES <files...>] [GLOB_FILES])
    # \brief\ Add an override linker script path to the target
    #
    # This can be used to override default linker script files with custom versions.
    #
    # For example, to use custom files in ${CMAKE_CURRENT_LIST_DIR}/extra_scripts instead of the default ones,
    # call pico_add_linker_script_override_path(TARGET ${CMAKE_CURRENT_LIST_DIR}/extra_scripts). This will
    # include the custom files first, overriding the default ones.
    #
    # It will not override prior paths set by pico_add_linker_script_override_path.
    #
    # \param\ TARGET The target to add the linker script override path to
    # \param\ PATH The path containing the overriding linker scripts
    # \param\ FILES The files in PATH - this is only used to calculate dependencies
    # \param\ GLOB_FILES Instead of passing FILES, just glob for all .incl files in directory
    function(pico_add_linker_script_override_path TARGET PATH)
        set(options GLOB_FILES)
        set(multiValueArgs FILES)
        cmake_parse_arguments(PARSE_ARGV 2 DEP "${options}" "${oneValueArgs}" "${multiValueArgs}")
        set_property(TARGET ${TARGET} APPEND PROPERTY PICO_TARGET_LINKER_SCRIPT_OVERRIDE_PATHS "${PATH}")
        if (DEP_GLOB_FILES)
            file(GLOB scripts CONFIGURE_DEPENDS "${PATH}/*.incl")
            foreach(script ${scripts})
                pico_add_link_depend(${TARGET} ${script})
            endforeach()
        else()
            foreach(script ${DEP_FILES})
                pico_add_link_depend(${TARGET} ${PATH}/${script})
            endforeach()
        endif()
    endfunction()

    # pico_override_flash_size(TARGET FLASH_SIZE)
    # \brief\ Override the FLASH size for a given target
    #
    # \param\ TARGET The target to override the FLASH size for
    # \param\ FLASH_SIZE The FLASH size to set for the target
    function(pico_override_flash_size TARGET FLASH_SIZE)
        set(PICO_FLASH_SIZE_BYTES_STRING "${FLASH_SIZE}")
        configure_file(${PICO_SDK_PATH}/src/rp2_common/pico_standard_link/pico_flash_region.template.ld ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}/pico_flash_region.ld)
        pico_add_linker_script_override_path(${TARGET} ${CMAKE_CURRENT_BINARY_DIR}/${TARGET} FILES pico_flash_region.ld)
        target_compile_definitions(${TARGET} PRIVATE "PICO_FLASH_SIZE_BYTES=${FLASH_SIZE}")
    endfunction()

    # pico_override_psram_size(TARGET PSRAM_SIZE)
    # \brief\ Override the PSRAM size for a given target
    #
    # \param\ TARGET The target to override the PSRAM size for
    # \param\ PSRAM_SIZE The PSRAM size to set for the target
    function(pico_override_psram_size TARGET PSRAM_SIZE)
        set(PICO_PSRAM_SIZE_BYTES_STRING "${PSRAM_SIZE}")
        configure_file(${PICO_SDK_PATH}/src/rp2_common/pico_standard_link/pico_psram_region.template.ld ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}/pico_psram_region.ld)
        pico_add_linker_script_override_path(${TARGET} ${CMAKE_CURRENT_BINARY_DIR}/${TARGET} FILES pico_psram_region.ld)
        target_compile_definitions(${TARGET} PRIVATE "PICO_PSRAM_SIZE_BYTES=${PSRAM_SIZE}")
    endfunction()

    # pico_set_binary_type(TARGET TYPE)
    # \brief\ Set the binary type for the target
    #
    # \param\ TYPE The binary type to set
    function(pico_set_binary_type TARGET TYPE)
        set_target_properties(${TARGET} PROPERTIES PICO_TARGET_BINARY_TYPE ${TYPE})
        pico_add_memmap_link_depends(${TARGET} ${TYPE})
    endfunction()

    # pico_include_in_generated_section(TARGET SECTION FILE)
    # \brief\ Append a linker script include file to a generated section of the linker script
    #
    # Arranges for FILE to be INCLUDEd inside the generated SECTION of the target's linker script.
    #
    # This is primarily intended for internal SDK use - users should instead override the
    # section_extra_SECTION.incl files, using pico_add_linker_script_override_path.
    #
    # \param\ TARGET  The target whose linker script should be modified
    # \param\ SECTION The linker script section name to append to (e.g. post_text)
    # \param\ FILE    Path to the linker script fragment to INCLUDE
    function(pico_include_in_generated_section TARGET SECTION FILE)
        get_target_property(already_generated ${TARGET} PICO_TARGET_LINKER_SCRIPT_INCLUDE_${SECTION})
        set_property(TARGET ${TARGET} APPEND PROPERTY PICO_TARGET_LINKER_SCRIPT_INCLUDE_${SECTION} "${FILE}")
        # Make sure directory is empty before generating files, to avoid stale overrides
        file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/generated/pico_standard_link/${TARGET})
        if (NOT already_generated)
            file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/generated/pico_standard_link/${TARGET}/section_generated_${SECTION}.incl
                CONTENT "INCLUDE \"$<JOIN:$<TARGET_PROPERTY:${TARGET},PICO_TARGET_LINKER_SCRIPT_INCLUDE_${SECTION}>,\"\nINCLUDE \">\""
            )
            pico_add_linker_script_override_path(${TARGET} ${CMAKE_BINARY_DIR}/generated/pico_standard_link/${TARGET})
        endif()
    endfunction()

    # pico_set_compile_definition(TARGET DEFINE VALUE)
    # \brief\ Set a compile definition on a target, guarding against conflicting redefinitions
    #
    # Equivalent to target_compile_definitions(TARGET PRIVATE DEFINE=VALUE) but tracks the value
    # as a target property so that a second call with the same DEFINE and the same VALUE is silently
    # ignored, while a second call with a different VALUE is a fatal error.
    #
    # \param\ TARGET The target to add the compile definition to
    # \param\ DEFINE The preprocessor symbol name
    # \param\ VALUE  The value to assign to the symbol
    function(pico_set_compile_definition TARGET DEFINE VALUE)
        get_target_property(already_set ${TARGET} PICO_TARGET_DEFINITION_${DEFINE})
        if (already_set MATCHES "NOTFOUND")
            set_target_properties(${TARGET} PROPERTIES PICO_TARGET_DEFINITION_${DEFINE} ${VALUE})
            target_compile_definitions(${TARGET} PRIVATE ${DEFINE}=${VALUE})
        else()
            if (NOT ${VALUE} STREQUAL ${already_set})
                message(FATAL_ERROR "Multiple calls to pico_set_compile_definition for ${DEFINE} with different values (${already_set} vs ${VALUE})")
            endif()
        endif()
    endfunction()

    # pico_generate_xip_ram_text_includes(TARGET)
    # \brief\ Configures the target to generate the "section_xip_ram_text_generated_includes.incl" file
    #         including all the requested linker script input section descriptions.
    #
    # The "section_xip_ram_text_generated_includes.incl" file contents vary depending on the
    # PICO_USE_XIP_CACHE_AS_RAM compile definition and what's been specified for the three INCLUDES
    # target properties via their pico_place_*_functions_in_xip_sram config functions:
    #     PICO_XIP_SRAM_TIME_CRITICAL_TEXT_INCLUDES
    #     PICO_XIP_SRAM_NOT_IN_FLASH_TEXT_INCLUDES
    #     PICO_XIP_SRAM_ALL_TEXT_INCLUDES
    #
    # When PICO_USE_XIP_CACHE_AS_RAM is undefined or false, no input section descriptions are included
    # in the generated file.
    #
    # When PICO_XIP_SRAM_ALL_TEXT_INCLUDES has been configured, it is assumed that not_in_flash and
    # time_critical functions should also be placed in XIP SRAM. If their respective INCLUDES target
    # properties have not been explicitly configured a "*" all-files pattern based input section
    # description is included for them.
    #
    # Otherwise, when PICO_XIP_SRAM_NOT_IN_FLASH_TEXT_INCLUDES has been configured, it is assumed that
    # time_critical functions should also be placed in XIP SRAM. If their respective INCLUDES target
    # property has not been explicitly configured a "*" all-files pattern based input section
    # description is included for them.
    #
    # Otherwise, when only PICO_XIP_SRAM_TIME_CRITICAL_TEXT_INCLUDES has been configured, no other
    # function section names are assumed for placement in XIP SRAM.
    #
    # Based on these cascading rules, starting at zero, a variable number of input section
    # descriptions are included in the generated file, for the three section name patterns:
    #     .time_critical.text*
    #     .not_in_flash.text*
    #     .text*
    #
    # And finally the generated file is wired up along with the linker and the INCLUDES_SIG properties
    # to support rebuilds and relinks when the input values change.
    #
    # \param\ TARGET The link target whose properties specify the requested input sections.
    function(pico_generate_xip_ram_text_includes TARGET)
        get_property(already_added TARGET ${TARGET} PROPERTY PICO_XIP_RAM_TEXT_INCLUDES_GENERATOR_ADDED SET)
        if (already_added)
            return()
        else()
            set_target_properties(${TARGET} PROPERTIES PICO_XIP_RAM_TEXT_INCLUDES_GENERATOR_ADDED TRUE)
        endif()

        set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated/pico_standard_link/${TARGET}")
        set(OUTPUT_FILE_NAME "section_xip_ram_text_generated_includes.incl")
        set(FULL_OUTPUT_FILE "${GENERATED_DIR}/${OUTPUT_FILE_NAME}")

        # extract truthy boolean value from compile definitions
        string(CONCAT PICO_USE_XIP_CACHE_AS_RAM
                "$<OR:"
                    "$<IN_LIST:PICO_USE_XIP_CACHE_AS_RAM,$<TARGET_PROPERTY:${TARGET},COMPILE_DEFINITIONS>>,"
                    "$<IN_LIST:PICO_USE_XIP_CACHE_AS_RAM=1,$<TARGET_PROPERTY:${TARGET},COMPILE_DEFINITIONS>>,"
                    "$<IN_LIST:PICO_USE_XIP_CACHE_AS_RAM=ON,$<TARGET_PROPERTY:${TARGET},COMPILE_DEFINITIONS>>,"
                    "$<IN_LIST:PICO_USE_XIP_CACHE_AS_RAM=TRUE,$<TARGET_PROPERTY:${TARGET},COMPILE_DEFINITIONS>>,"
                    "$<IN_LIST:PICO_USE_XIP_CACHE_AS_RAM=YES,$<TARGET_PROPERTY:${TARGET},COMPILE_DEFINITIONS>>,"
                    "$<IN_LIST:PICO_USE_XIP_CACHE_AS_RAM=Y,$<TARGET_PROPERTY:${TARGET},COMPILE_DEFINITIONS>>"
                ">")

        set(TIME_CRITICAL_DECLARED "$<NOT:$<STREQUAL:$<TARGET_PROPERTY:${TARGET},PICO_XIP_SRAM_TIME_CRITICAL_TEXT_INCLUDES>,>>")
        set(NOT_IN_FLASH_DECLARED  "$<NOT:$<STREQUAL:$<TARGET_PROPERTY:${TARGET},PICO_XIP_SRAM_NOT_IN_FLASH_TEXT_INCLUDES>,>>")
        set(ALL_DECLARED           "$<NOT:$<STREQUAL:$<TARGET_PROPERTY:${TARGET},PICO_XIP_SRAM_ALL_TEXT_INCLUDES>,>>")

        # Cascading Include groups: (time_critical), (time_critical && not_in_flash), (time_critical && not_in_flash && all)
        set(COND_TIME_CRITICAL "$<OR:${TIME_CRITICAL_DECLARED},${NOT_IN_FLASH_DECLARED},${ALL_DECLARED}>")
        set(COND_NOT_IN_FLASH  "$<OR:${NOT_IN_FLASH_DECLARED},${ALL_DECLARED}>")
        set(COND_ALL           "${ALL_DECLARED}")

        set(INCLUDE_ALL_TIME_CRITICAL_TEXT "    *(.time_critical.text*)\n")
        set(INCLUDE_ALL_NOT_IN_FLASH_TEXT  "    *(.not_in_flash.text*)\n")
        set(INCLUDE_ALL_TEXT               "    *(.text*)\n")

        # assemble, clean-up, and flatten
        string(CONCAT PICO_XIP_RAM_TEXT_INCLUDES
                "/* Generated by pico-sdk: object file includes for the linker .xip_ram_text section */\n"
                "$<IF:$<AND:${PICO_USE_XIP_CACHE_AS_RAM},${COND_TIME_CRITICAL}>,$<IF:${TIME_CRITICAL_DECLARED},$<TARGET_PROPERTY:${TARGET},PICO_XIP_SRAM_TIME_CRITICAL_TEXT_INCLUDES>,${INCLUDE_ALL_TIME_CRITICAL_TEXT}>,\n>"
                "$<IF:$<AND:${PICO_USE_XIP_CACHE_AS_RAM},${COND_NOT_IN_FLASH}>,$<IF:${NOT_IN_FLASH_DECLARED},$<TARGET_PROPERTY:${TARGET},PICO_XIP_SRAM_NOT_IN_FLASH_TEXT_INCLUDES>,${INCLUDE_ALL_NOT_IN_FLASH_TEXT}>,\n>"
                "$<IF:$<AND:${PICO_USE_XIP_CACHE_AS_RAM},${COND_ALL}>,$<IF:${ALL_DECLARED},$<TARGET_PROPERTY:${TARGET},PICO_XIP_SRAM_ALL_TEXT_INCLUDES>,${INCLUDE_ALL_TEXT}>,\n>"
        )

        file(MAKE_DIRECTORY "${GENERATED_DIR}")
        file(GENERATE OUTPUT "${FULL_OUTPUT_FILE}" CONTENT "${PICO_XIP_RAM_TEXT_INCLUDES}")
        set_target_properties(${TARGET} PROPERTIES LINK_DEPENDS "${FULL_OUTPUT_FILE}")

        pico_add_linker_script_override_path(${TARGET} "${GENERATED_DIR}")
        # force a re-link if any of the pico_include_*_functions_in_xip_sram() function's ARGS values changed
        target_link_options(${TARGET} PRIVATE
                "-Wl,--defsym=PICO_XIP_TRACK_TC_$<TARGET_PROPERTY:${TARGET},PICO_XIP_SRAM_TIME_CRITICAL_TEXT_INCLUDES_SIG>=0"
                "-Wl,--defsym=PICO_XIP_TRACK_NF_$<TARGET_PROPERTY:${TARGET},PICO_XIP_SRAM_NOT_IN_FLASH_TEXT_INCLUDES_SIG>=0"
                "-Wl,--defsym=PICO_XIP_TRACK_AL_$<TARGET_PROPERTY:${TARGET},PICO_XIP_SRAM_ALL_TEXT_INCLUDES_SIG>=0"
        )
    endfunction()

    # pico_add_linker_input_section_descriptions(TARGET INPUT_SECTION_DESCRIPTIONS SECTION_NAME_PATTERN [FILENAMES])
    # \brief\ Composes input section descriptions into the ${INPUT_SECTION_DESCRIPTIONS} property based
    #         on the optional FILENAMES list and SECTION_NAME_PATTERN provided.
    #
    # This function may be invoked multiple times to specify additional FILENAMES, the generated input
    # section descriptions will be appended to the ${INPUT_SECTION_DESCRIPTIONS} property. An effort is
    # made to avoid appending duplicate descriptions.
    #
    # In addition to the ${INPUT_SECTION_DESCRIPTIONS} property, this function will also maintains a
    # signature property ${INPUT_SECTION_DESCRIPTIONS}_SIG with its value tracking the base property.
    #
    # \param\ TARGET The target of the properties being populated
    # \param\ INPUT_SECTION_DESCRIPTIONS The name of the target property accumulating linker script
    #         input section descriptions.
    # \param\ SECTION_NAME_PATTERN The input section name pattern used for all linker script input
    #         section descriptions
    # \param\ FILENAMES An optional user specified list of object (or archive) filenames used as the
    #         basis for generating file specific linker script input section file patterns. If none are
    #         specified the "*" all-files pattern is assumed.
    function(pico_add_linker_input_section_descriptions TARGET INPUT_SECTION_DESCRIPTIONS SECTION_NAME_PATTERN)
        get_target_property(existing_section_descriptions ${TARGET} ${INPUT_SECTION_DESCRIPTIONS})

        # generate new section_descriptions content
        if ("${ARGN}" STREQUAL "")
            # if no FILE_NAMES specified, default to the "*" any file pattern
            set(section_descriptions "    *(${SECTION_NAME_PATTERN})\n")

            # fast path escape for simple case: no OBJECT_FILES specified while setting initial property values
            if (existing_section_descriptions MATCHES "-NOTFOUND$")
                set_target_properties(${TARGET} PROPERTIES
                        ${INPUT_SECTION_DESCRIPTIONS} "${section_descriptions}"
                        ${INPUT_SECTION_DESCRIPTIONS}_SIG "0"
                )
                return()
            endif()
        else()
            # otherwise translate the specified filenames into input section file patterns:
            # take the list of filename args, transform into linker includes, and flatten into one multi-line block
            set(section_descriptions ${ARGN})
            # support object archive files using the archive:file pattern
            list(TRANSFORM section_descriptions REPLACE "^(.+(\\.a|\\.lib))$" "\\1:*")
            # includes matching TARGET_OBJECTS for specified object filenames with and without a path context
            list(TRANSFORM section_descriptions REPLACE "^(.+)$" "    */\\1(${SECTION_NAME_PATTERN})\\n    *\\1(EXCLUDE_FILE(*?\\1) ${SECTION_NAME_PATTERN})\\n")
            list(JOIN section_descriptions "" section_descriptions)
        endif()

        # update the INPUT_SECTION_DESCRIPTIONS by adding the ${section_descriptions} but skipping duplicate content
        string(FIND "${existing_section_descriptions}" "${section_descriptions}" found_index)
        if(found_index EQUAL -1)
            # append section_descriptions to the ${INPUT_SECTION_DESCRIPTIONS} property
            set_property(TARGET ${TARGET} APPEND_STRING PROPERTY ${INPUT_SECTION_DESCRIPTIONS} "${section_descriptions}")

            # calculate an up-to-date signature value for the aggregate ${INPUT_SECTION_DESCRIPTIONS} value
            get_target_property(aggregate_section_descriptions ${TARGET} ${INPUT_SECTION_DESCRIPTIONS})
            string(MD5 md5_sig "${aggregate_section_descriptions}")
            string(SUBSTRING "${md5_sig}" 0 8 short_sig)
            set_target_properties(${TARGET} PROPERTIES ${INPUT_SECTION_DESCRIPTIONS}_SIG "${short_sig}")
        endif()
    endfunction()

    # pico_place_time_critical_functions_in_xip_sram(TARGET [OBJECT_FILES])
    # \brief\ Places ".time_critical.text*" sections of the target into XIP SRAM.
    #
    # Configures the target to place functions marked __time_critical_func() into XIP SRAM rather than
    # normal SRAM, as it has dedicated AHB ports which are otherwise unused in no_flash and copy_to_ram
    # binaries. This sets PICO_USE_XIP_CACHE_AS_RAM=1, composes the required linker script input
    # section descriptions, and populates the ".xip_ram_text" linker script output section with those
    # descriptions using the "section_xip_ram_text_generated_includes.incl" file generated via
    # pico_generate_xip_ram_text_includes().
    #
    # This function may be invoked multiple times to specify additional OBJECT_FILES for the generated
    # input section descriptions.
    #
    # Using this function with flash binaries is not recommended, as their performance depends
    # critically on XIP hardware being used to cache the instructions stored in flash, and this function
    # disables the XIP cache when it sets PICO_USE_XIP_CACHE_AS_RAM=1.
    #
    # \param\ TARGET The target whose time_critical functions should run from XIP SRAM
    # \param\ OBJECT_FILES An optional user specified list of object (or archive) filenames used as the
    #         basis for generating file specific linker script input section file patterns for the
    #         ".time_critical.text*" section names. If none are specified the "*" all-files pattern is
    #         assumed.
    function(pico_place_time_critical_functions_in_xip_sram TARGET)
        pico_set_compile_definition(${TARGET} PICO_USE_XIP_CACHE_AS_RAM 1) # may be set elsewhere too
        pico_add_linker_input_section_descriptions(${TARGET} PICO_XIP_SRAM_TIME_CRITICAL_TEXT_INCLUDES ".time_critical.text*" "${ARGN}")
        pico_generate_xip_ram_text_includes(${TARGET})
    endfunction()

    # pico_place_not_in_flash_functions_in_xip_sram(TARGET [OBJECT_FILES])
    # \brief\ Places both ".not_in_flash.text*" and ".time_critical.text*" sections of the target into XIP SRAM.
    #
    # Configures the target to place functions marked __not_in_flash_func() into XIP SRAM rather than
    # normal SRAM, as it has dedicated AHB ports which are otherwise unused in no_flash and copy_to_ram
    # binaries. This sets PICO_USE_XIP_CACHE_AS_RAM=1, composes the required linker script input
    # section descriptions, and populates the ".xip_ram_text" linker script output section with those
    # descriptions using the "section_xip_ram_text_generated_includes.incl" file generated via
    # pico_generate_xip_ram_text_includes().
    #
    # This function may be invoked multiple times to specify additional OBJECT_FILES for the generated input
    # section descriptions. By default functions marked __time_critical_func() will also be placed into
    # XIP SRAM when this function is used. That behavior may be overridden by specifying a dummy filename in
    # a call to the pico_place_time_critical_functions_in_xip_sram() function and avoiding its default "*"
    # all-files matching pattern.
    #
    # Using this function with flash binaries is not recommended, as their performance depends
    # critically on XIP hardware being used to cache the instructions stored in flash, and this function
    # disables the XIP cache when it sets PICO_USE_XIP_CACHE_AS_RAM=1.
    #
    # \param\ TARGET The target whose not_in_flash functions should run from XIP SRAM
    # \param\ OBJECT_FILES An optional user specified list of object (or archive) filenames used as the
    #         basis for generating file specific linker script input section file patterns for the
    #         ".not_in_flash.text*" section names. If none are specified the "*" all-files pattern is
    #         assumed.
    function(pico_place_not_in_flash_functions_in_xip_sram TARGET)
        pico_set_compile_definition(${TARGET} PICO_USE_XIP_CACHE_AS_RAM 1) # may be set elsewhere too
        pico_add_linker_input_section_descriptions(${TARGET} PICO_XIP_SRAM_NOT_IN_FLASH_TEXT_INCLUDES ".not_in_flash.text*" "${ARGN}")
        pico_generate_xip_ram_text_includes(${TARGET})
    endfunction()

    # pico_place_all_functions_in_xip_sram(TARGET [OBJECT_FILES])
    # \brief\ Places ".text*", ".not_in_flash.text*", and ".time_critical.text*" sections of the target into XIP SRAM.
    #
    # Configures the target to place all functions into XIP SRAM rather than normal SRAM, as it has
    # dedicated AHB ports which are otherwise unused in no_flash and copy_to_ram binaries. This sets
    # PICO_USE_XIP_CACHE_AS_RAM=1, composes the required linker script input section descriptions, and
    # populates the ".xip_ram_text" linker script output section with those descriptions using the
    # "section_xip_ram_text_generated_includes.incl" file generated via pico_generate_xip_ram_text_includes().
    #
    # This function may be invoked multiple times to specify additional OBJECT_FILES for the generated input
    # section descriptions. By default functions marked __not_in_flash_func() or __time_critical_func() will
    # also be placed into XIP SRAM when this function is used. That behavior may be overridden by specifying a
    # dummy filename in a call to the pico_place_not_in_flash_functions_in_xip_sram() function or the
    # pico_place_time_critical_functions_in_xip_sram() function respectively and avoiding their default "*"
    # all-files matching pattern.
    #
    # Using this function with flash binaries is not recommended, as their performance depends
    # critically on XIP hardware being used to cache the instructions stored in flash, and this function
    # disables the XIP cache when it sets PICO_USE_XIP_CACHE_AS_RAM=1.
    #
    # \param\ TARGET The target whose functions should run from XIP SRAM
    # \param\ OBJECT_FILES An optional user specified list of object (or archive) filenames used as the
    #         basis for generating file specific linker script input section file patterns for the
    #         ".text*" section names. If none are specified the "*" all-files pattern is assumed.
    function(pico_place_all_functions_in_xip_sram TARGET)
        pico_set_compile_definition(${TARGET} PICO_USE_XIP_CACHE_AS_RAM 1) # may be set elsewhere too
        pico_add_linker_input_section_descriptions(${TARGET} PICO_XIP_SRAM_ALL_TEXT_INCLUDES ".text*" "${ARGN}")
        pico_generate_xip_ram_text_includes(${TARGET})
    endfunction()

    # PICO_CMAKE_CONFIG: PICO_DEFAULT_BINARY_TYPE, The default binary type to use, type=string, default=default, group=build
    # slightly messy as we support both the preferred PICO_DEFAULT_BINARY_TYPE and the individual variables
    if (NOT PICO_DEFAULT_BINARY_TYPE)
        if (PICO_NO_FLASH)
            set(PICO_DEFAULT_BINARY_TYPE no_flash)
        elseif (PICO_USE_BLOCKED_RAM)
            set(PICO_DEFAULT_BINARY_TYPE blocked_ram)
        elseif (PICO_COPY_TO_RAM)
            set(PICO_DEFAULT_BINARY_TYPE copy_to_ram)
        else()
            set(PICO_DEFAULT_BINARY_TYPE default)
        endif()
    else()
        # we must set the individual variables here, as they are used in generator expressions,
        # but also for our checks below
        if (PICO_DEFAULT_BINARY_TYPE STREQUAL no_flash)
            set(PICO_NO_FLASH 1)
        endif()
        if (PICO_DEFAULT_BINARY_TYPE STREQUAL blocked_ram)
            set(PICO_USE_BLOCKED_RAM 1)
        endif()
        if (PICO_DEFAULT_BINARY_TYPE STREQUAL copy_to_ram)
            set(PICO_COPY_TO_RAM 1)
        endif()
    endif()
    if ((PICO_NO_FLASH AND PICO_USE_BLOCKED_RAM) OR
        (PICO_USE_BLOCKED_RAM AND PICO_COPY_TO_RAM) OR
        (PICO_COPY_TO_RAM AND PICO_NO_FLASH))
        message(FATAL_ERROR "Conflicting binary types specified amongst PICO_DEFAULT_BINARY_TYPE, PICO_NO_FLASH, PICO_USE_BLOCKED_RAM and PICO_COPY_TO_RAM")
    endif()

    # todo only needed if not using a custom linker script
    if (NOT PICO_LINKER_SCRIPT_PATH)
        set(PICO_LINKER_SCRIPT_PATH "THIS_IS_THE_UNSET_PICO_LINKER_SCRIPT_PATH")
    endif()
    # configure the flash size in pico_flash_region.ld
    if (NOT PICO_FLASH_SIZE_BYTES)
        if (PICO_DEFAULT_FLASH_SIZE_BYTES)
            set(PICO_FLASH_SIZE_BYTES ${PICO_DEFAULT_FLASH_SIZE_BYTES})
        else()
            set(PICO_FLASH_SIZE_BYTES "2 * 1024 * 1024")
        endif()
    endif()
    # since linker script can handle expressions; may as well leave it as one
    #math(EXPR PICO_FLASH_SIZE_BYTES_STRING "${PICO_FLASH_SIZE_BYTES}" OUTPUT_FORMAT HEXADECIMAL)
    set(PICO_FLASH_SIZE_BYTES_STRING "${PICO_FLASH_SIZE_BYTES}")
    configure_file(${CMAKE_CURRENT_LIST_DIR}/pico_flash_region.template.ld ${CMAKE_CURRENT_BINARY_DIR}/pico_flash_region.ld)
    # add include path for linker scripts to find it
    list(APPEND PICO_LINKER_SCRIPT_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR})
    pico_promote_common_scope_vars()

    # add PSRAM region if it is enabled
    if (PICO_PSRAM_SIZE_BYTES)
        set(PICO_PSRAM_SIZE_BYTES_STRING "${PICO_PSRAM_SIZE_BYTES}")
    else()
        set(PICO_PSRAM_SIZE_BYTES_STRING "0")
    endif()
    configure_file(${CMAKE_CURRENT_LIST_DIR}/pico_psram_region.template.ld ${CMAKE_CURRENT_BINARY_DIR}/pico_psram_region.ld)

    # add override paths from PICO_TARGET_LINKER_SCRIPT_OVERRIDE_PATHS
    target_link_options(pico_standard_link INTERFACE "LINKER:$<$<BOOL:$<TARGET_PROPERTY:PICO_TARGET_LINKER_SCRIPT_OVERRIDE_PATHS>>:-L$<JOIN:$<TARGET_PROPERTY:PICO_TARGET_LINKER_SCRIPT_OVERRIDE_PATHS>,,-L>>")

    # add include paths from PICO_LINKER_SCRIPT_INCLUDE_DIRS
    foreach(DIR IN LISTS PICO_LINKER_SCRIPT_INCLUDE_DIRS)
        target_link_options(pico_standard_link INTERFACE "LINKER:-L${DIR}")
    endforeach()

    # add default locations script, so they can be referenced by pico_set_linker_script_var variables
    target_link_options(pico_standard_link INTERFACE "LINKER:--script=${PICO_LINKER_DEFAULT_LOCATIONS_PATH}")

    # add variables set by pico_set_linker_script_var function
    target_link_options(pico_standard_link INTERFACE "LINKER:$<$<BOOL:$<TARGET_PROPERTY:PICO_TARGET_LINKER_SCRIPT_VARS>>:--defsym=$<JOIN:$<TARGET_PROPERTY:PICO_TARGET_LINKER_SCRIPT_VARS>,,--defsym=>>")

    # LINKER script will be PICO_TARGET_LINKER_SCRIPT if set on target, or ${CMAKE_CURRENT_LIST_DIR}/memmap_foo.ld
    # if PICO_TARGET_BINARY_TYPE is set to foo on the target, otherwise ${CMAKE_CURRENT_LIST_DIR}/memmap_${PICO_DEFAULT_BINARY_TYPE).ld
    set(_LINKER_SCRIPT_EXPRESSION "$<IF:$<BOOL:$<TARGET_PROPERTY:PICO_TARGET_LINKER_SCRIPT>>,$<TARGET_PROPERTY:PICO_TARGET_LINKER_SCRIPT>,${PICO_LINKER_SCRIPT_PATH}/memmap_$<IF:$<STREQUAL:$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>,>,${PICO_DEFAULT_BINARY_TYPE},$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>>.ld>")
    target_link_options(pico_standard_link INTERFACE
        # use -T instead of --script here, as picolibc.specs looks for any -T not --linker to suppress injection of its own linker script
        "-T${_LINKER_SCRIPT_EXPRESSION}"
            )
    pico_add_link_depend(pico_standard_link ${_LINKER_SCRIPT_EXPRESSION})
    unset(_LINKER_SCRIPT_EXPRESSION)

    pico_add_memmap_link_depends(pico_standard_link ${PICO_DEFAULT_BINARY_TYPE})

    # PICO_NO_FLASH will be set based on PICO_TARGET_BUILD_TYPE target property being equal to no_flash if set, otherwise to the value of the PICO_NO_FLASH cmake variable unless PICO_TARGET_TYPE is set to something else
    set(NO_FLASH_BINARY_TYPES no_flash xip_ram)
    set(COPY_TO_RAM_BINARY_TYPES copy_to_ram)
    set(XIP_RAM_BINARY_TYPES xip_ram)

    # PICO_NO_FLASH will be set based on PICO_TARGET_BUILD_TYPE target property being in NO_FLASH_BINARY_TYPES if set, otherwise to the value of the PICO_NO_FLASH cmake variable unless PICO_TARGET_TYPE is set to something else
    # PICO_BUILD_DEFINE: PICO_NO_FLASH, whether this is a 'no_flash' build, type=bool, default=0, but dependent on CMake options, group=pico_standard_link
    target_compile_definitions(pico_standard_link INTERFACE PICO_NO_FLASH=$<IF:$<IN_LIST:$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>,${NO_FLASH_BINARY_TYPES}>,1,$<AND:$<BOOL:${PICO_NO_FLASH}>,$<STREQUAL:,$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>>>>)
    # PICO_USE_BLOCKED_RAM will be set based on PICO_TARGET_BUILD_TYPE target property being equal to use_blocked_ram if set, otherwise to the value of the PICO_USE_BLOCKED_RAM cmake variable unless PICO_TARGET_TYPE is set to something else
    # PICO_BUILD_DEFINE: PICO_USE_BLOCKED_RAM, whether this is a 'blocked_ram' build, type=bool, default=0, but dependent on CMake options, group=pico_standard_link
    target_compile_definitions(pico_standard_link INTERFACE PICO_USE_BLOCKED_RAM=$<IF:$<STREQUAL:$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>,use_blocked_ram>,1,$<AND:$<BOOL:${PICO_USE_BLOCKED_RAM}>,$<STREQUAL:,$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>>>>)
    # PICO_COPY_TO_RAM will be set based on PICO_TARGET_BUILD_TYPE target property being in COPY_TO_RAM_BINARY_TYPES if set, otherwise to the value of the PICO_COPY_TO_RAM cmake variable unless PICO_TARGET_TYPE is set to something else
    # PICO_BUILD_DEFINE: PICO_COPY_TO_RAM, whether this is a 'copy_to_ram' build, type=bool, default=0, but dependent on CMake options, group=pico_standard_link
    target_compile_definitions(pico_standard_link INTERFACE PICO_COPY_TO_RAM=$<IF:$<IN_LIST:$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>,${COPY_TO_RAM_BINARY_TYPES}>,1,$<AND:$<BOOL:${PICO_COPY_TO_RAM}>,$<STREQUAL:,$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>>>>)
    # PICO_XIP_RAM will be set based on PICO_TARGET_BUILD_TYPE target property being in XIP_RAM_BINARY_TYPES if set, otherwise to the value of the PICO_XIP_RAM cmake variable unless PICO_TARGET_TYPE is set to something else
    # PICO_BUILD_DEFINE: PICO_XIP_RAM, whether this is a 'xip_ram' build, type=bool, default=0, but dependent on CMake options, group=pico_standard_link
    target_compile_definitions(pico_standard_link INTERFACE PICO_XIP_RAM=$<IF:$<IN_LIST:$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>,${XIP_RAM_BINARY_TYPES}>,1,$<AND:$<BOOL:${PICO_XIP_RAM}>,$<STREQUAL:,$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>>>>)

    target_compile_definitions(pico_standard_link INTERFACE PICO_CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}")
    if (PICO_DEOPTIMIZED_DEBUG AND "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        target_compile_definitions(pico_standard_link INTERFACE PICO_DEOPTIMIZED_DEBUG=1)
    endif()

    # this (arguably wrong) code is restored for 1.5.1 as setting -nostartfiles on many C++ binaries causes link errors. see issue #1368
    # -nostartfiles will be added if PICO_NO_FLASH would be defined to 1
    target_link_options(pico_standard_link INTERFACE $<$<IF:$<STREQUAL:$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>,no_flash>,1,$<AND:$<BOOL:${PICO_NO_FLASH}>,$<STREQUAL:,$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>>>>:-nostartfiles>)
    # boot_stage2 will be linked if PICO_NO_FLASH would be defined to 0; note if boot_stage2 headers not present, then boot_stage2 is omitted from build anyway
    if (TARGET boot_stage2_headers)
        target_link_libraries(pico_standard_link INTERFACE $<$<NOT:$<IF:$<STREQUAL:$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>,no_flash>,1,$<AND:$<BOOL:${PICO_NO_FLASH}>,$<STREQUAL:,$<TARGET_PROPERTY:PICO_TARGET_BINARY_TYPE>>>>>:$<IF:$<BOOL:$<TARGET_PROPERTY:PICO_TARGET_BOOT_STAGE2>>,$<TARGET_PROPERTY:PICO_TARGET_BOOT_STAGE2>,bs2_default>_library>)
    endif()

    # PICO_CMAKE_CONFIG: PICO_USE_DEFAULT_MAX_PAGE_SIZE, Don't shrink linker max page to 4096, type=bool, default=0, advanced=true, group=pico_standard_link
    if (NOT PICO_USE_DEFAULT_MAX_PAGE_SIZE)
        target_link_options(pico_standard_link INTERFACE "LINKER:-z,max-page-size=4096")
    endif()
    # done in compiler now
    #target_link_options(pico_standard_link INTERFACE "LINKER:--build-id=none")

    # this line occasionally useful for debugging ... todo maybe make a PICO_ var
    # target_compile_options(pico_standard_link INTERFACE --save-temps) #debugging only

    # PICO_CMAKE_CONFIG: PICO_NO_GC_SECTIONS, Disable `-ffunction-sections` `-fdata-sections` and `--gc-sections`, type=bool, default=0, advanced=true, group=pico_standard_link
    if (NOT PICO_NO_GC_SECTIONS)
        target_compile_options(pico_standard_link INTERFACE -ffunction-sections -fdata-sections)
        target_link_options(pico_standard_link INTERFACE "LINKER:--gc-sections")
    endif()

    if (PICO_C_COMPILER_IS_GNU)
        # Ignore warnings about rwx segments introduced in binutils 2.39
        execute_process(COMMAND ${CMAKE_C_COMPILER} -print-prog-name=ld RESULT_VARIABLE RUN_C_RESULT OUTPUT_VARIABLE FULL_LD_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        if (${RUN_C_RESULT} EQUAL 0)
            execute_process(COMMAND ${FULL_LD_PATH} --help RESULT_VARIABLE RUN_LD_RESULT OUTPUT_VARIABLE LD_HELP_OUTPUT
                OUTPUT_STRIP_TRAILING_WHITESPACE)
            if (${RUN_LD_RESULT} EQUAL 0)
                    set(RWX_WARNING "no-warn-rwx-segments")
                    string(FIND "${LD_HELP_OUTPUT}" "${RWX_WARNING}" LD_RWX_WARNING_SUPPORTED)
                    if (${LD_RWX_WARNING_SUPPORTED} GREATER -1)
                        target_link_options(pico_standard_link INTERFACE "LINKER:--${RWX_WARNING}")
                    endif()
            endif()
        endif()
    endif()
endif()
