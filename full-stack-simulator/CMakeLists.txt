cmake_minimum_required(VERSION 3.16)
project(full_stack_network_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(include)

add_executable(full_stack_network_simulator
    src/main.cpp
    src/app/Services.cpp
    src/analysis/DomainAnalyzer.cpp
    src/core/Logger.cpp
    src/devices/Bridge.cpp
    src/devices/Device.cpp
    src/devices/Host.cpp
    src/devices/Hub.cpp
    src/devices/Router.cpp
    src/devices/Switch.cpp
    src/physical/Link.cpp
    src/protocols/Protocols.cpp
    src/simulator/Scenarios.cpp
)
