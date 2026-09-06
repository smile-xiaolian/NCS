cmake_minimum_required(VERSION 3.22)
project(NCS VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
if(MSVC)
  add_compile_options(/utf-8 /W4)
else()
  add_compile_options(-Wall -Wextra)
endif()
find_package(Qt6 6.2 REQUIRED COMPONENTS Core Widgets Sql Charts WebEngineWidgets)
add_subdirectory(core)
add_subdirectory(client_user)
add_subdirectory(client_admin)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_region_report.py
    ${CMAKE_BINARY_DIR}/bin/generate_region_report.py
    COPYONLY
)
