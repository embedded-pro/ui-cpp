#pragma once

#include "ui/model/FieldSpec.hpp"
#include "ui/model/TableSpec.hpp"
#include <span>

namespace ui::model
{
    struct FormSpec
    {
        std::span<const GroupSpec> groups;
        std::span<const FieldSpec> fields;
        std::span<const ActionSpec> actions;
        std::span<const TableSpec> tables;
    };
}
