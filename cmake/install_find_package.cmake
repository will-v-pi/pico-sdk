

if (INSTALL_DEBIAN_PACKAGE_LAYOUT)
    # Debian system layout under /usr/src/pico-sdk
    set(EXTRA_NOT_FOUND_MESSAGE " Install the \"pico-sdk-source\" package.")
    set(pico_sdk_install_dir "/usr/src/pico-sdk")
    set(config_file_install_dir "${PICO_SDK_PATH}/cmake")
    set(SKIP_SDK_INSTALL TRUE) # the debian packaging handles this instead
else()
    # Normal install
    include(GNUInstallDirs)
    # Have to use BINDIR/../xxx because there is no SRCDIR, and LIBDIR sometimes has a suffix which can cause issues
    set(pico_sdk_install_dir "${CMAKE_INSTALL_FULL_BINDIR}/../src/pico_sdk")
    set(config_file_install_dir "${CMAKE_INSTALL_FULL_BINDIR}/../lib/cmake/pico_sdk")
    set(SKIP_SDK_INSTALL FALSE)
endif()

# Create a ConfigVersion.cmake file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/pico_sdk-config-version.cmake
    VERSION ${PICO_SDK_VERSION_STRING}
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/pico_sdk-config.cmake.template
    ${CMAKE_CURRENT_BINARY_DIR}/pico_sdk-config.cmake
    INSTALL_DESTINATION ${config_file_install_dir}
)

# Install the config and configversion
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/pico_sdk-config-version.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/pico_sdk-config.cmake
    DESTINATION ${config_file_install_dir}
)

if (SKIP_SDK_INSTALL)
    return()
endif()

# Install SDK files
function(sdk_should_install_file path result)
    if((NOT EXISTS "${PICO_SDK_PATH}/${path}") OR (IS_DIRECTORY "${PICO_SDK_PATH}/${path}")) # skip any files that don't exist, or directories
        set(${result} FALSE PARENT_SCOPE)
    elseif ("${path}" MATCHES "^\\.") # exclude dotfiles
        set(${result} FALSE PARENT_SCOPE)
    elseif ("${path}" MATCHES "lib/btstack/port") # exclude unecessary btstack files
        set(${result} FALSE PARENT_SCOPE)
    elseif ("${path}" MATCHES "\\.bazel" OR "${path}" MATCHES "\\.bzl" OR "${path}" MATCHES "^bazel") # exclude bazel files
        set(${result} FALSE PARENT_SCOPE)
    else()
        set(${result} TRUE PARENT_SCOPE)
    endif()
endfunction()

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Install all tracked files, with exclusions in sdk_should_install_file
    execute_process(COMMAND ${GIT_EXECUTABLE} ls-files --recurse-submodules
                    WORKING_DIRECTORY ${PICO_SDK_PATH}
                    OUTPUT_VARIABLE GIT_LS_FILES)
    string(REPLACE "\n" ";" GIT_LS_FILES "${GIT_LS_FILES}")
    set(SDK_INSTALL_FILES "")
    foreach (FILE IN LISTS GIT_LS_FILES)
        sdk_should_install_file("${FILE}" should_install)
        if (should_install)
            get_filename_component(DIRECTORY "${FILE}" DIRECTORY)
            install(FILES
                "${PICO_SDK_PATH}/${FILE}"
                DESTINATION "${pico_sdk_install_dir}/${DIRECTORY}"
            )
        endif()
    endforeach()
endif()
