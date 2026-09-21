ui_add_library(ui.model
    SOURCES
        FieldId.hpp
        FieldSpec.hpp
        FormModel.cpp
        FormModel.hpp
        FormSpec.hpp
        TableModel.cpp
        TableSpec.hpp
    LINK
        ui.core
        ui.theme
)

add_subdirectory(test)
