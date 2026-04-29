file(GLOB SRCS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cxx"
)

foreach(src ${SRCS})
    get_filename_component(exe_name ${src} NAME_WE)

    add_executable(${exe_name} ${src})
endforeach()