#pragma once

#include "ui/model/FieldSpec.hpp"
#include "ui/model/TableSpec.hpp"
#include <cstdint>
#include <span>

namespace ui::model
{
    enum class FormLayout : std::uint8_t
    {
        Stacked,
        Inline
    };

    struct FormSpec
    {
        std::span<const GroupSpec> groups;
        std::span<const FieldSpec> fields;
        std::span<const ActionSpec> actions;
        std::span<const TableSpec> tables;
        FormLayout layout{ FormLayout::Stacked };
    };
}
