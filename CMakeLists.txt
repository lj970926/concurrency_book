# SPDX-FileCopyrightText: 2026 lj970926
#
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16)

project(concurrency_book LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src)

option(CONCURRENCY_BOOK_BUILD_TESTS "Build concurrency_book tests" ON)

if(CONCURRENCY_BOOK_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

add_subdirectory(scripts)
