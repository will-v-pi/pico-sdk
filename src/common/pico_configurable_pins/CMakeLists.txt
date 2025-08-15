pico_add_library(pico_configurable_pins)

target_sources(pico_configurable_pins INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/configurable_pins.c
)

target_include_directories(pico_configurable_pins_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

pico_mirrored_target_link_libraries(pico_configurable_pins INTERFACE
        pico_binary_info
)
