# Add device library
add_library(device STATIC)

target_sources(device PRIVATE
    # HAL source
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/hal/src/clock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/hal/src/context_switch.s
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/hal/src/miscutil.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/hal/src/sleep.c

    # Periph Driver source
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/BlueNRG1_Periph_Driver/src/BlueNRG1_gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/BlueNRG1_Periph_Driver/src/BlueNRG1_sysCtrl.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/BlueNRG1_Periph_Driver/src/BlueNRG1_uart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/BlueNRG1_Periph_Driver/src/BlueNRG1_mft.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/BlueNRG1_Periph_Driver/src/BlueNRG1_adc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/BlueNRG1_Periph_Driver/src/BlueNRG1_wdg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/BlueNRG1_Periph_Driver/src/misc.c

    # Bluetooth_LE stubes source
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/Bluetooth_LE/src/stack_dle_api_stubs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/Bluetooth_LE/src/stack_priv_api_stubs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/Bluetooth_LE/src/stack_sc_api_stubs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/Bluetooth_LE/src/stack_slvonly_api_stubs.c

    # CMSIS source
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/CMSIS/Device/ST/BlueNRG1/Source/system_BlueNRG1.c   
)

target_sources(device PUBLIC
    # HAL source
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/hal/src/osal.c

    # Periph Driver source
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/BlueNRG1_Periph_Driver/src/BlueNRG1_flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/BlueNRG1_Periph_Driver/src/BlueNRG1_pka.c

    # Bluetooth_LE source
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/Bluetooth_LE/src/stack_user_cfg.c
)

target_include_directories(device PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/CMSIS/Device/ST/BlueNRG1/Include/
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/CMSIS/Include/
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/hal/inc/
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/BlueNRG1_Periph_Driver/inc/
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/BLE_Application/layers_inc/
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/Bluetooth_LE/inc/
)

target_include_directories(device PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(device PRIVATE
    -w # Disable all warning
)

target_link_libraries(device PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/Bluetooth_LE/library/libbluenrg1_stack.a
    ${CMAKE_CURRENT_SOURCE_DIR}/bluenrg-2/cryptolib/libcrypto.a
)