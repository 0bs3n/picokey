cmake_minimum_required(VERSION 3.13)

add_compile_definitions(CFG_TUSB_OS=OPT_OS_PICO)

# set(CMAKE_CONFIGURATION_TYPES "Debug" CACHE STRING "" FORCE)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(pikey_proj)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

file(GLOB SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "src/*.c")

# rest of your project
add_executable(pikey
    ${SOURCES}
)

target_include_directories(pikey PRIVATE include)

# enable usb output, disable uart output
pico_enable_stdio_usb(pikey 0)
pico_enable_stdio_uart(pikey 1)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(pikey)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(pikey pico_stdlib tinyusb_device)
