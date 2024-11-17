cmake_minimum_required(VERSION 3.10)






# Set VERSION from cmake argument or the last git hash.
if(NOT DEFINED VERSION)
    # Get the last git tag
    execute_process(
        COMMAND git describe --tags --abbrev=0
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_LAST_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # If no tag is found, set a default version
    if(NOT GIT_LAST_TAG)
        set(GIT_LAST_TAG "0.0.0")
    endif()

    # Remove the 'v' prefix from the tag
    string(REGEX REPLACE "^v" "" GIT_LAST_TAG ${GIT_LAST_TAG})

    # Get the last git hash
    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_LAST_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Convert the git hash to a decimal number
    math(EXPR GIT_LAST_HASH_DEC "0x${GIT_LAST_HASH}" OUTPUT_FORMAT DECIMAL)

    # Set the VERSION variable
    set(VERSION "${GIT_LAST_TAG}.${GIT_LAST_HASH_DEC}")
endif()


# Define projects
# ##############################################################################
project(ble_smart_lock
    VERSION ${VERSION}
    DESCRIPTION "Bluetooth LE Smart Lock"
    HOMEPAGE_URL "https://github.com/antoine163/ble-smart-lock"
    LANGUAGES C ASM)
message(STATUS "Project version: ${CMAKE_PROJECT_VERSION}")

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Enable verbose to debug cmake
# set(CMAKE_VERBOSE_MAKEFILE ON)

# Write .clangd file for clangd pluging in vscodium
file(WRITE ".clangd"
     "CompileFlags:\n"
     "    CompilationDatabase: ${CMAKE_BINARY_DIR}\n")

# Compilation definitions
# ##############################################################################

add_compile_definitions(
    -DBLUENRG2_DEVICE
    -DBLE_STACK_CONFIGURATION=BLE_STACK_FULL_CONFIGURATION
    -DPROJECT_VERSION="${CMAKE_PROJECT_VERSION}"
    $<$<CONFIG:Debug>:-DDEBUG>
)

if(MODEL_BLUENRG STREQUAL "M2SP")
    message(STATUS "Build for MODEL:BLUENRG-M2SP.")

    # Additional definitions for BLUENRG-M2SP
    add_compile_definitions(
        -DHS_SPEED_XTAL=HS_SPEED_XTAL_32MHZ
        -DLS_SOURCE=LS_SOURCE_INTERNAL_RO
        -DSMPS_INDUCTOR=SMPS_INDUCTOR_NONE
    )
elseif(MODEL_BLUENRG STREQUAL "M2SA")
    message(STATUS "Build for MODEL:BLUENRG-M2SA.")

    # Additional definitions for BLUENRG-M2SA
    add_compile_definitions(
        -DHS_SPEED_XTAL=HS_SPEED_XTAL_32MHZ
        -DLS_SOURCE=LS_SOURCE_EXTERNAL_32KHZ
        -DSMPS_INDUCTOR=SMPS_INDUCTOR_10uH
    )
else()
    message(FATAL_ERROR "Unsupported BlueNRG module choice ! Please define BLUENRG on 'M2SP' or 'M2SA'")
endif()

# Compilation options
# ##############################################################################

# Get sysroot path
execute_process(
        COMMAND ${CMAKE_C_COMPILER} -print-sysroot
        OUTPUT_VARIABLE SYSROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

# Compile options
add_compile_options(
    -W -Wall -Wextra -Wpedantic
    -fmessage-length=0

    -ffunction-sections
    -fdata-sections

    -mcpu=cortex-m0
    -mthumb
    -mfloat-abi=soft

    $<$<COMPILE_LANGUAGE:ASM>:-x>
    $<$<COMPILE_LANGUAGE:ASM>:assembler-with-cpp>

    $<$<CONFIG:Debug>:-g3>

    -I${SYSROOT}/include # To add sysroot to compile_commands.json for clangd
)

# Link options
# ##############################################################################
add_link_options(
    -specs=nano.specs

    # -nostdlib
    # -nodefaultlibs
    # -nostartfiles
    -Wl,-Map=${PROJECT_NAME}.map
    -Wl,--cref
    -Wl,--gc-sections

    -mcpu=cortex-m0
    -mthumb
    -mfloat-abi=soft

    -T${CMAKE_SOURCE_DIR}/src/board/linker/BlueNRG2.ld
    -Wl,--print-memory-usage
)

# Add all sources
# ##############################################################################
add_subdirectory(src)

# Output files
# ##############################################################################
add_executable(${CMAKE_PROJECT_NAME}.elf src/app/main.c)
target_link_libraries(${CMAKE_PROJECT_NAME}.elf app)

# Additional commands
# ##############################################################################

# Print executable size
add_custom_command(TARGET ${CMAKE_PROJECT_NAME}.elf
    POST_BUILD
    COMMAND ${CMAKE_SIZE_UTIL} ${CMAKE_PROJECT_NAME}.elf
)

# Create hex and bin file
add_custom_command(TARGET ${CMAKE_PROJECT_NAME}.elf
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${CMAKE_PROJECT_NAME}.elf ${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_PROJECT_NAME}.elf ${CMAKE_PROJECT_NAME}.bin
)

# disassemble
add_custom_target(disassemble
    DEPENDS ${CMAKE_PROJECT_NAME}.elf
    COMMAND ${CMAKE_OBJDUMP} -hd ${CMAKE_PROJECT_NAME}.elf > ${CMAKE_PROJECT_NAME}.s
    COMMAND code ${CMAKE_PROJECT_NAME}.s
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Release install hex file
install(
    FILES ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.hex
    DESTINATION ${CMAKE_SOURCE_DIR}/release
    RENAME ${CMAKE_PROJECT_NAME}_${MODEL_BLUENRG}_${VERSION}.hex)

# Configure gdb scripts
set(TARGET_NAME ${CMAKE_PROJECT_NAME}.elf)
set(OCD_SCRIPT ${CMAKE_SOURCE_DIR}/tools/openocd/bluenrg-2.cfg)
configure_file(
    ${CMAKE_SOURCE_DIR}/tools/gdb/gdbinit_debug.cfg.in
    ${CMAKE_BINARY_DIR}/tools/gdb/gdbinit_debug.cfg
)
configure_file(
    ${CMAKE_SOURCE_DIR}/tools/gdb/gdbinit_load.cfg.in
    ${CMAKE_BINARY_DIR}/tools/gdb/gdbinit_load.cfg
)

# Run gdb with cgdb to debug
add_custom_target(cgdb
    DEPENDS ${CMAKE_PROJECT_NAME}.elf
    COMMAND cgdb -d ${CMAKE_GDB}
    --command=${CMAKE_BINARY_DIR}/tools/gdb/gdbinit_debug.cfg
    ${CMAKE_PROJECT_NAME}.elf
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Run gdb with to load
add_custom_target(load
    DEPENDS ${CMAKE_PROJECT_NAME}.elf
    COMMAND ${CMAKE_GDB}
    -x ${CMAKE_BINARY_DIR}/tools/gdb/gdbinit_load.cfg
    ${CMAKE_PROJECT_NAME}.elf
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Flash with openocd
add_custom_target(flash
    DEPENDS ${CMAKE_PROJECT_NAME}.elf
    COMMAND openocd -d0 -f ${CMAKE_SOURCE_DIR}/tools/openocd/bluenrg-2.cfg
    -c "program ${CMAKE_PROJECT_NAME}.elf verify reset exit; shutdown"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
