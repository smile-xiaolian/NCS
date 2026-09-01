# ============================================================
# client_user/CMakeLists.txt - 充电用户端
# ============================================================

project(ncs_user)

# 收集 UI 源文件
set(USER_SOURCES
    main.cpp
    ui/LoginWindow.cpp
    ui/MainWindow.cpp
    ui/StationListPage.cpp
    ui/StationDetailPage.cpp
    ui/MapPage.cpp
    ui/UserCenterPage.cpp
    ui/ChargePage.cpp
    ui/OrderSettlePage.cpp
)

set(USER_HEADERS
    ui/LoginWindow.h
    ui/MainWindow.h
    ui/StationListPage.h
    ui/StationDetailPage.h
    ui/MapPage.h
    ui/UserCenterPage.h
    ui/ChargePage.h
    ui/OrderSettlePage.h
)

# 创建可执行文件
add_executable(ncs_user WIN32 ${USER_SOURCES} ${USER_HEADERS})

# 链接 Qt 和 core 库
target_link_libraries(ncs_user
    core_lib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::WebEngineWidgets
)

# 包含头文件路径
target_include_directories(ncs_user
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui
)

# Windows 下设置窗口为 GUI 应用 (不显示控制台)
if(MSVC)
    set_target_properties(ncs_user PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# 复制数据库和资源到运行目录 (可选)
add_custom_command(TARGET ncs_user POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/data/charge_platform.db
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data/
    COMMENT "Copying database file..."
)