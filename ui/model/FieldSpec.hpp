#pragma once

#include "ui/model/FieldId.hpp"
#include "ui/theme/Theme.hpp"
#include <cstdint>
#include <span>
#include <string_view>

namespace ui::model
{
    enum class FieldKind : std::uint8_t
    {
        Number,
        Integer,
        Choice,
        Toggle,
        ReadOut
    };

    // The defaults are QDoubleSpinBox's own, so a field that states no range behaves exactly as the
    // hand-written panel it replaces did by omission rather than by intent.
    struct NumberTraits
    {
        double minimum{ 0.0 };
        double maximum{ 99.99 };
        double step{ 1.0 };
        double initial{ 0.0 };
        std::uint8_t decimals{ 2 };
    };

    // data defaults to the option's own index, which is what lets one read path serve both the
    // panels that carry user data and the ones that switch on the selected index.
    struct OptionSpec
    {
        std::string_view label;
        std::int64_t data{ -1 };
    };

    // Every conditional field across the eleven consumer applications is "this choice is on one of
    // these options", so a mask over the selection expresses all of them - including the negated
    // form, which is the complement of a mask rather than a second operator.
    struct Condition
    {
        FieldId field{ noField };
        std::uint32_t selectionMask{ 0 };

        [[nodiscard]] constexpr bool Unconditional() const
        {
            return field == noField;
        }
    };

    // Labels are views over storage the caller owns, which in practice is always a string literal.
    // Building one with std::format and letting it die yields a dangling view with no diagnostic.
    struct FieldSpec
    {
        FieldId id{};
        GroupId group{ noGroup };
        FieldKind kind{ FieldKind::Number };
        std::string_view label;
        std::string_view suffix;
        NumberTraits number{};
        std::span<const OptionSpec> options;
        Condition visibleWhen{};
        Condition enabledWhen{};
    };

    struct GroupSpec
    {
        GroupId id{};
        std::string_view title;
        Condition visibleWhen{};
    };

    struct ActionSpec
    {
        ActionId id{};
        std::string_view label;
        theme::ButtonRole buttonRole{ theme::ButtonRole::Default };
        std::uint16_t minimumHeight{ 0 };
    };
}
