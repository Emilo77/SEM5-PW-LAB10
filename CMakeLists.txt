cmake_minimum_required (VERSION 3.1)
project (MIMUW-SIGNALS C)

set(CMAKE_CXX_STANDARD "17")
set(CMAKE_C_STANDARD "11")
set(CMAKE_C_FLAGS "-g -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter")

add_library(err err.c)

function(add_example_shmem_program target_name)
  add_executable(${target_name} "${target_name}.c")
  target_link_libraries(${target_name} err)
  target_link_libraries(${target_name} pthread)
  target_link_libraries(${target_name} rt)
endfunction()

add_example_shmem_program(get_memory)
add_example_shmem_program(send_message)
add_example_shmem_program(send_message_no_inheritance)
add_example_shmem_program(unnamed_memory)
add_example_shmem_program(send_message_protected)
add_example_shmem_program(unnamed_memory_protected)

add_example_shmem_program(gifts)
