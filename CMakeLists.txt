cmake_minimum_required(VERSION 3.6)
project(zadanie2_cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(SOURCE_FILES err.c err.h siktacka.h siktacka.cpp data_structures.h data_structures.cpp siktacka-client.cpp events.h events.cpp)
add_executable(siktacka-client ${SOURCE_FILES})

set(SOURCE_FILES err.c err.h siktacka.h siktacka.cpp data_structures.h data_structures.cpp siktacka-server.cpp server.h server.cpp events.h events.cpp)
add_executable(siktacka-server ${SOURCE_FILES})