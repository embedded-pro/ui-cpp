#pragma once

#include <cstdint>

namespace ui::model
{
    // Three distinct wrappers rather than aliases for one integer type: a field, a group and an
    // action are all small ints that would otherwise interchange silently at every call site.
    struct FieldId
    {
        std::uint16_t value{ 0 };

        [[nodiscard]] friend constexpr bool operator==(FieldId lhs, FieldId rhs) = default;
    };

    struct GroupId
    {
        std::uint16_t value{ 0 };

        [[nodiscard]] friend constexpr bool operator==(GroupId lhs, GroupId rhs) = default;
    };

    struct ActionId
    {
        std::uint16_t value{ 0 };

        [[nodiscard]] friend constexpr bool operator==(ActionId lhs, ActionId rhs) = default;
    };

    inline constexpr FieldId noField{ 0xFFFFu };
    inline constexpr GroupId noGroup{ 0xFFFFu };
    inline constexpr ActionId noAction{ 0xFFFFu };
}
