cmake_minimum_required(VERSION 3.15)
project(kryppi)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /O2)
else()
    add_compile_options(-Wall -O2)
endif()

# Help CMake find macOS SDK headers
if(APPLE)
    execute_process(
        COMMAND xcrun --show-sdk-path
        OUTPUT_VARIABLE MACOS_SDK_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(CURSES_INCLUDE_PATH "${MACOS_SDK_PATH}/usr/include")
endif()

set(CURSES_NEED_WIDE TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Curses REQUIRED)

add_executable(kryppi
    src/main.cpp
    src/draw_tui.cpp
    src/run_tui.cpp
    src/msg_handler.cpp
    ../udp-socket/src/udp_socket.cpp
    ../udp-socket/src/json.cpp
    ../udp-socket/src/kryp.cpp
)

target_include_directories(kryppi PRIVATE
    ${CURSES_INCLUDE_DIR}
    include
    ../udp-socket/inc
)

target_link_libraries(kryppi PRIVATE ${CURSES_LIBRARIES})
