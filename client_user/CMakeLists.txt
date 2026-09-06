

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

# 引入 WebEngineWidgets 模块 (UC-U-04)
target_link_libraries(ncs_user PRIVATE core_lib Qt6::Widgets Qt6::Sql Qt6::WebEngineWidgets)
set_target_properties(ncs_user PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 内嵌 db/schema.sql 建表脚本 (UC-D-01)
qt_add_resources(ncs_user "ncs_schema"
    PREFIX "/"
    BASE "${CMAKE_SOURCE_DIR}/core"
    FILES "${CMAKE_SOURCE_DIR}/core/db/schema.sql")