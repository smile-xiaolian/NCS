add_executable(platform_app
    main.cpp
    PlatformController.cpp
    PlatformController.h
    DemoPiles.cpp
    DemoPiles.h
    ProjectPileDb.cpp
    ProjectPileDb.h
    TriggerHttpServer.cpp
    TriggerHttpServer.h
    ui/PlatformMainWindow.cpp
    ui/PlatformMainWindow.h
)

target_link_libraries(platform_app PRIVATE device_link_core pile_logic pile_gui Qt6::Widgets Qt6::Sql)
