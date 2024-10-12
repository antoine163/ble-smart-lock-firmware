# Add application library
add_library(board STATIC)

target_sources(board PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/board.c
)

target_sources(board PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/uart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/pwm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/adc.c
)

target_include_directories(board PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(board PUBLIC
    device middleware
)
