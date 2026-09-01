add_executable(ncs_admin main.cpp)
target_link_libraries(ncs_admin PRIVATE core_lib Qt6::Widgets Qt6::Charts Qt6::Sql)
set_target_properties(ncs_admin PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
