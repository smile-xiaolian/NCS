add_library(core_lib STATIC database/DatabaseManager.cpp service/PlatformService.cpp utils/Haversine.cpp)
target_include_directories(core_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(core_lib PUBLIC Qt6::Core Qt6::Sql)
