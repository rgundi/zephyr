
add_library(CMSIS INTERFACE)

target_include_directories(CMSIS INTERFACE ${PROJECT_SOURCE_DIR}/include/cmsis)

zephyr_library()
zephyr_library_sources(cmsis_os2.c)

zephyr_library_link_libraries(CMSIS)
target_link_libraries(CMSIS INTERFACE zephyr_interface)
