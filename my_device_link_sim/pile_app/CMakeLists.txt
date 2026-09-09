# 桩业务逻辑抽成库:platform_app 的演示模式(内置虚拟桩)同样复用这套状态机
add_library(pile_logic STATIC
    PileController.cpp
    PileController.h
)

target_include_directories(pile_logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pile_logic PUBLIC device_link_core Qt6::Core)

# 桩端 GUI(窗口 + 内置模拟平台):platform_app 打开"桩窗口"时同样复用
add_library(pile_gui STATIC
    MockPlatform.cpp
    MockPlatform.h
    ui/PileMainWindow.cpp
    ui/PileMainWindow.h
)

target_include_directories(pile_gui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pile_gui PUBLIC pile_logic Qt6::Widgets)

add_executable(pile_app main.cpp)

target_link_libraries(pile_app PRIVATE pile_gui)
