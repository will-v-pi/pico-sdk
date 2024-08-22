
if (DEFINED PICO_SDK_PATH)
    pico_sdk_init()
else()
    message(FATAL_ERROR "Must use the toolchain file at ${CMAKE_CURRENT_LIST_DIR}/pico_toolchain_file.cmake, else pico-sdk project will not build")
endif()
