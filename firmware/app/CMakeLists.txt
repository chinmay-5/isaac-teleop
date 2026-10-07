cmake_minimum_required(VERSION 3.20.0)

list(APPEND ZEPHYR_EXTRA_MODULES
    ${CMAKE_CURRENT_SOURCE_DIR}/../modules/micro_ros_zephyr_module/modules/libmicroros
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(isaac_teleop)

target_sources(app PRIVATE
    src/main.c
)
