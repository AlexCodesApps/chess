cmake_minimum_required(VERSION 3.10)
project(chess C)
enable_testing()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

if (ANDROID)
    add_subdirectory(android-project/app/jni/SDL)
    add_subdirectory(android-project/app/jni/SDL_image)
else()
    find_package(SDL3 CONFIG REQUIRED)
    find_package(SDL3_image CONFIG REQUIRED)
endif()

set(SOURCES
    src/chess.c
    src/concurrency.c
    src/display.c
    src/include/bench.h
    src/include/chess.h
    src/include/concurrency.h
    src/include/display.h
    src/include/ints.h
    src/include/maths.h
    src/include/state.h
    src/include/str.h
    src/include/texture.h
    src/include/uci.h
    src/state.c
    src/str.c
    src/texture.c
    src/uci.c)

if (ANDROID)
    add_library(main SHARED main.c ${SOURCES})
    target_link_libraries(main PRIVATE SDL3::SDL3 SDL3_image::SDL3_image)
else()
    add_executable(main main.c ${SOURCES})
    target_link_libraries(main PRIVATE SDL3::SDL3 SDL3_image::SDL3_image)
endif()


add_executable(
    tests
    test/count_moves.c
    test/fen.c
    test/test.c
    ${SOURCES}
)

target_link_libraries(tests PRIVATE SDL3::SDL3 SDL3_image::SDL3_image)

set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")
set(ASSETS_DEST "${CMAKE_BINARY_DIR}/assets")

add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets"
            "${CMAKE_BINARY_DIR}/assets"
    COMMENT "Copying assets"
)

add_test(NAME tests COMMAND tests)
