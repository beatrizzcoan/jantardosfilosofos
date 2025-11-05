cmake_minimum_required(VERSION 4.0)
project(jantardosfilosofos C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(jantardosfilosofos
        common.h
        coordenador.c
        filosofo.c)
