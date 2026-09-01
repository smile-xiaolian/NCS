add_executable(ncs_user main.cpp)
target_link_libraries(ncs_user PRIVATE core_lib Qt6::Widgets Qt6::Sql)
set_target_properties(ncs_user PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
