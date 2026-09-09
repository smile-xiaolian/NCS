cmake_minimum_required(VERSION 3.19)
project(my_device_link_sim VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(MSVC)
    add_compile_options(/utf-8 /W3 /permissive-)
else()
    add_compile_options(-Wall -Wextra)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network WebSockets Test Sql)

enable_testing()

add_subdirectory(core)
add_subdirectory(pile_app)
add_subdirectory(platform_app)
add_subdirectory(tests)

message(STATUS "my_device_link_sim: platform_app + pile_app ready")
