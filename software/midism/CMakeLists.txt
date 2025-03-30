# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
#set(PICO_BOARD pico_w CACHE STRING "Board type")
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(midism C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# sub directories
file(GLOB MIDI_SOURCES "midi/*.cpp" "midi/channel/*.cpp" "midi/voice/*.cpp")
aux_source_directory(hal HAL_SOURCES)
aux_source_directory(usb USB_SOURCES)

# link all sources and generate executable
add_executable(midism
    main.cpp
    Debugger.cpp
    ${MIDI_SOURCES}
    ${HAL_SOURCES}
    ${USB_SOURCES}
)

# include directories
target_include_directories(midism PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/hal
    ${CMAKE_CURRENT_SOURCE_DIR}/usb
    ${CMAKE_CURRENT_SOURCE_DIR}/midi
    ${CMAKE_CURRENT_SOURCE_DIR}/midi/channel
    ${CMAKE_CURRENT_SOURCE_DIR}/midi/voice
)

pico_set_program_name(midism "midism")
pico_set_program_version(midism "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_usb(midism 0)
pico_enable_stdio_uart(midism 1)

# Add the standard library to the build
target_link_libraries(midism
    pico_stdlib
    pico_multicore
    tinyusb_device
    tinyusb_board
)      

# Add the standard include files to the build
target_include_directories(midism PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(midism)

pico_add_extra_outputs(midism)

# Doxygen target
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()

# Clang-Format target
find_program(CLANG_FORMAT_EXECUTABLE clang-format)
if (CLANG_FORMAT_EXECUTABLE)
    # Collect all .cpp and .h files
    file(GLOB_RECURSE ALL_SOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/hal/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/hal/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/midi/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/midi/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/midi/channel/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/midi/channel/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/midi/voice/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/midi/voice/*.h"
    )
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${ALL_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Formatting all source files with clang-format"
    )
else()
    message(WARNING "clang-format not found. Format target will not be available.")
endif()
