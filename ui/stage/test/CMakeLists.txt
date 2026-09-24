ui_add_test(ui.stage_test
    SOURCES
        TestMesh.cpp
        TestPrimitives.cpp
        TestSceneGraph.cpp
        TestStage.cpp
        TestStageRenderer.cpp
        TestStageView.cpp
        TestStl.cpp
    LINK
        ui.stage
        ui.backend.recording
)

# Its own executable: it replaces the global operator new to count allocations, which must not
# leak into the other suites.
ui_add_test(ui.stage_allocation_test
    SOURCES
        TestStageAllocation.cpp
    LINK
        ui.stage
)
