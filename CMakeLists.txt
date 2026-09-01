# ============================================================
# NCS 充电桩管理平台 - 顶层 CMakeLists.txt
# 适用: Windows (MSVC 2019) + Linux (GCC 11)
# ============================================================

cmake_minimum_required(VERSION 3.22)
project(NCS VERSION 1.0.0 LANGUAGES CXX)

# -------------------- 全局配置 --------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)   # Qt 元对象编译器
set(CMAKE_AUTORCC ON)   # Qt 资源编译器
set(CMAKE_AUTOUIC ON)   # Qt UI 编译器

# -------------------- 平台适配 --------------------
if(MSVC)
    # Windows: 使用 UTF-8 编码 (NFR-C-03)
    add_compile_options(/utf-8)
    add_compile_options(/W4)
    # 禁用安全警告 (C4996)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    # Linux / GCC
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
endif()

# -------------------- 寻找 Qt6 --------------------
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Network
    Sql
    WebEngineWidgets
    Charts
)

# 如果 Qt6 找不到，尝试 Qt5（备用）
if(NOT Qt6_FOUND)
    message(WARNING "Qt6 not found, trying Qt5...")
    find_package(Qt5 REQUIRED COMPONENTS
        Core Widgets Network Sql WebEngineWidgets Charts
    )
    set(QT_VERSION_MAJOR 5)
else()
    set(QT_VERSION_MAJOR 6)
endif()

message(STATUS "Using Qt${QT_VERSION_MAJOR}")

# -------------------- 子目录 --------------------
# 1. 核心库 (被用户端和管理端共享)
add_subdirectory(core)

# 2. 用户端程序
add_subdirectory(client_user)

# 3. 管理端程序
add_subdirectory(client_admin)

# -------------------- 全局输出目录 --------------------
# 所有可执行文件和库都输出到 build/bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# -------------------- 打印配置信息 --------------------
message(STATUS "========================================")
message(STATUS "  NCS 项目配置完成")
message(STATUS "  构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ 标准: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Qt 版本: ${QT_VERSION_MAJOR}")
message(STATUS "========================================")