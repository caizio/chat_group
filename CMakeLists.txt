cmake_minimum_required(VERSION 3.15)

project(caizi_chat_project)

add_definitions("-O0 -g -ggdb -Wno-unused-variable")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

INCLUDE_DIRECTORIES(server client)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

add_subdirectory(server)
add_subdirectory(client)

add_executable(server.exe main_server.cpp)
target_link_libraries(server_src)