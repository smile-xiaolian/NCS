add_executable(ncs_user main.cpp)
target_link_libraries(ncs_user PRIVATE core_lib Qt6::Widgets Qt6::Sql)
set_target_properties(ncs_user PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 内嵌 db/schema.sql 建表脚本 (UC-D-01)
qt_add_resources(ncs_user "ncs_schema"
    PREFIX "/"
    BASE "${CMAKE_SOURCE_DIR}/core"
    FILES "${CMAKE_SOURCE_DIR}/core/db/schema.sql")
