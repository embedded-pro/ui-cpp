add_executable(ui.stage_demo)
target_sources(ui.stage_demo PRIVATE Main.cpp)
target_link_libraries(ui.stage_demo PRIVATE ui.stage ui.backend.qt)
ui_set_warnings(ui.stage_demo)
