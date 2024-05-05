
cmake_minimum_required(VERSION 3.0)

project(server_ex_2b C)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-value -Wno-unused-local-typedefs")

add_executable(server_ex_2b servermain.c calcLib.h calcLib.c)