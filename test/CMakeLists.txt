cmake_minimum_required(VERSION 3.4)

add_custom_target(build_test)

add_executable(psx_test psx_test.cc)
add_test(NAME psx COMMAND $<TARGET_FILE:psx_test>)
add_dependencies(build_test psx_test)
