# ============================================================
# core/CMakeLists.txt - 核心共享库
# ============================================================

project(core_lib)

# 收集所有源文件
set(CORE_SOURCES
    database/DatabaseManager.cpp
    service/UserService.cpp
    service/StationService.cpp
    service/ChargeService.cpp
    service/StatsService.cpp
    utils/Logger.cpp
    utils/Haversine.cpp
)

set(CORE_HEADERS
    database/DatabaseManager.h
    service/UserService.h
    service/StationService.h
    service/ChargeService.h
    service/StatsService.h
    models/User.h
    models/Station.h
    models/Charger.h
    models/Order.h
    utils/Logger.h
    utils/Haversine.h
)

# 创建静态库 (方便链接)
add_library(core_lib STATIC ${CORE_SOURCES} ${CORE_HEADERS})

# 链接 Qt 库
target_link_libraries(core_lib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Network
)

# 包含目录 (供其他模块 #include "core/xxx.h")
target_include_directories(core_lib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# 让其他模块能找到 core_lib
set(CORE_LIB core_lib PARENT_SCOPE)