add_library(device_link_core STATIC
    MessageFrame.cpp
    ActionDispatcher.cpp
    PendingRequestTable.cpp
    DeviceConnection.cpp
    DeviceRegistry.cpp
)

target_include_directories(device_link_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(device_link_core PUBLIC Qt6::Core Qt6::Network Qt6::WebSockets)
