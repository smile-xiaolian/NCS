# ============================================================
# client_admin/CMakeLists.txt - PC 管理端
# ============================================================

project(ncs_admin)

set(ADMIN_SOURCES
    main.cpp
    ui/AdminLoginWindow.cpp
    ui/AdminMainWindow.cpp
    ui/RevenuePage.cpp
    ui/StatusOverviewPage.cpp
    ui/ChargerManagePage.cpp
    ui/StationManagePage.cpp
    ui/UserManagePage.cpp
    ui/PredictPage.cpp
)

set(ADMIN_HEADERS
    ui/AdminLoginWindow.h
    ui/AdminMainWindow.h
    ui/RevenuePage.h
    ui/StatusOverviewPage.h
    ui/ChargerManagePage.h
    ui/StationManagePage.h
    ui/UserManagePage.h
    ui/PredictPage.h
)

add_executable(ncs_admin WIN32 ${ADMIN_SOURCES} ${ADMIN_HEADERS})

target_link_libraries(ncs_admin
    core_lib
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Charts
)

target_include_directories(ncs_admin
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ui
)

if(MSVC)
    set_target_properties(ncs_admin PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()