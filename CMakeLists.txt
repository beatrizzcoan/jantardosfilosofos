cmake_minimum_required(VERSION 3.10)
project(jantardosfilosofos C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

find_package(Threads REQUIRED)

add_executable(coordenador coordenador.c common.h)

target_link_libraries(coordenador PRIVATE Threads::Threads)

add_executable(filosofo filosofo.c common.h)
