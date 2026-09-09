add_executable(device_link_tests
    test_core.cpp
)

target_link_libraries(device_link_tests PRIVATE device_link_core Qt6::Test)

add_test(NAME device_link_tests COMMAND device_link_tests)
