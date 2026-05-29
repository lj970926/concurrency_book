# SPDX-FileCopyrightText: 2026 lj970926
#
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16)

project(concurrency_book LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(CONCURRENCY_BOOK_BUILD_TESTS "Build concurrency_book tests" ON)

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
endif()

add_subdirectory(src)


if(CONCURRENCY_BOOK_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

add_subdirectory(scripts)
