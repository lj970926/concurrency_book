cmake_minimum_required(VERSION 3.16)

project(concurrency_book LANGUAGES CXX)

add_library(concurrency_book INTERFACE)
target_include_directories(concurrency_book INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(concurrency_book INTERFACE cxx_std_17)

option(CONCURRENCY_BOOK_BUILD_TESTS "Build concurrency_book tests" ON)

if(CONCURRENCY_BOOK_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

add_subdirectory(scripts)
