ui_add_test(ui.backend.quick3d_test CUSTOM_MAIN
    SOURCES
        Main.cpp
        TestGltfInspection.cpp
        TestQuick3DGeometry.cpp
        TestQuick3DModule.cpp
    LINK
        ui.backend.quick3d_plugin
        ui.backend.quick3d
        ui.stage
)

if (TARGET ui.backend.quick3d_test)
    target_compile_definitions(ui.backend.quick3d_test PRIVATE GLTF_TEST_DATA="${CMAKE_CURRENT_LIST_DIR}/data")
endif()
