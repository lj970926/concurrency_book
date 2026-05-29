# SPDX-FileCopyrightText: 2026 lj970926
#
# SPDX-License-Identifier: MIT

file(GLOB SRCS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
)
add_library(concurrency_book SHARED ${SRCS})
target_include_directories(concurrency_book PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_features(concurrency_book INTERFACE cxx_std_17)
target_link_libraries(concurrency_book PUBLIC atomic)
