cmake_minimum_required(VERSION 3.10)
project(ChatApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(server
    src/server/main.cpp
    src/server/Server.cpp
    src/server/Logger.cpp
)

add_executable(client
    src/client/main.cpp
    src/client/Client.cpp
)