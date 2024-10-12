# Add application library
add_library(app STATIC)

target_sources(app PRIVATE
)

target_sources(app PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/tasks/tasks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tasks/taskApp/taskApp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tasks/taskBle/taskBle.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tasks/taskLight/taskLight.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tasks/taskTerm/taskTerm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/syscall.c
)

target_include_directories(app PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(app PUBLIC
    board
)
