set(CMAKE_AUTORCC ON)
add_executable(ncs_admin
    main.cpp
    ui/AdminMainWindow.cpp
    ui/RevenuePage.cpp
    ui/ChargerStatusPage.cpp
    ui/ChargerManagePage.cpp
    ui/DeviceLinkHook.cpp
    ui/StationManagePage.cpp
    ui/UserManagePage.cpp
    ui/resources.qrc
)
target_link_libraries(ncs_admin PRIVATE core_lib Qt6::Widgets Qt6::Charts Qt6::Sql Qt6::Network)
set_target_properties(ncs_admin PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 内嵌 db/schema.sql 建表脚本(与 client_user 一致):
# 管理端可能先于用户端首启建库/迁移,缺此资源会报"无法加载内置建表脚本"
qt_add_resources(ncs_admin "ncs_admin_schema"
    PREFIX "/"
    BASE "${CMAKE_SOURCE_DIR}/core"
    FILES "${CMAKE_SOURCE_DIR}/core/db/schema.sql"
)
