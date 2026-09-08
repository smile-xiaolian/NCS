add_executable(ncs_user
    main.cpp
    ui/LoginWindow.cpp
    ui/StationListPage.cpp
    ui/StationDetailPage.cpp
    ui/UserCenterPage.cpp
    ui/OrderListPage.cpp
    ui/SettlePage.cpp
    ui/OrderTicketDialog.cpp
    ui/MapWindow.cpp
    ui/UserMainWindow.cpp
)

target_include_directories(ncs_user PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 1. 添加 Qt6::Network 模块依赖
target_link_libraries(ncs_user PRIVATE 
    core_lib 
    Qt6::Widgets 
    Qt6::Sql 
    Qt6::WebEngineWidgets 
    Qt6::Network
    Qt6::Multimedia
    Qt6::MultimediaWidgets
)
set_target_properties(ncs_user PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 2. 内嵌 db/schema.sql 建表脚本
qt_add_resources(ncs_user "ncs_schema"
    PREFIX "/"
    BASE "${CMAKE_SOURCE_DIR}/core"
    FILES "${CMAKE_SOURCE_DIR}/core/db/schema.sql"
)

# 3. 独立内嵌 HTML 导航地图模板
qt_add_resources(ncs_user "ncs_map_resources"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES "resources/map_template.html"
)
