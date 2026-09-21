#pragma once

#include "ui/core/Callback.hpp"
#include "ui/model/FormSpec.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace ui::model
{
    struct FieldValue
    {
        double number{ 0.0 };
        std::uint16_t selection{ 0 };
        bool flag{ false };
    };

    enum class ViolationKind : std::uint8_t
    {
        BelowMinimum,
        AboveMaximum,
        NoSelection,
        NoRows
    };

    struct Violation
    {
        FieldId field{ noField };
        ViolationKind kind{ ViolationKind::BelowMinimum };

        [[nodiscard]] friend constexpr bool operator==(const Violation& lhs, const Violation& rhs) = default;
    };

    // Binds a spec to storage the caller owns. Nothing here allocates: the spec arrays, the value
    // array and the table cells all live in the consumer, which is what lets a form be built in a
    // constructor and then never touch the heap again.
    class FormModel
    {
    public:
        FormModel(const FormSpec& spec, std::span<FieldValue> values, std::span<TableModel> tables);

        [[nodiscard]] const FormSpec& Spec() const;
        [[nodiscard]] const FieldSpec& Field(FieldId id) const;

        [[nodiscard]] double Number(FieldId id) const;
        [[nodiscard]] float Float(FieldId id) const;
        [[nodiscard]] std::int64_t Integer(FieldId id) const;
        [[nodiscard]] std::size_t Count(FieldId id) const;
        [[nodiscard]] std::size_t Selection(FieldId id) const;
        [[nodiscard]] std::int64_t SelectedData(FieldId id) const;
        [[nodiscard]] bool Flag(FieldId id) const;

        void SetNumber(FieldId id, double value);
        void SetSelection(FieldId id, std::size_t index);
        bool SelectByData(FieldId id, std::int64_t data);
        void SetFlag(FieldId id, bool value);

        [[nodiscard]] bool IsVisible(FieldId id) const;
        [[nodiscard]] bool IsEnabled(FieldId id) const;
        [[nodiscard]] bool IsGroupVisible(GroupId id) const;

        [[nodiscard]] std::size_t TableCount() const;
        [[nodiscard]] TableModel& Table(std::size_t index);
        [[nodiscard]] const TableModel& Table(std::size_t index) const;

        void ResetToDefaults();
        [[nodiscard]] std::optional<Violation> Validate() const;

        void TriggerAction(ActionId id);
        void NotifyTableChanged();

        // Fires whenever a value changes, including when the backend's own control caused it. The
        // backend suppresses its own echo; the model does not try to guess who wrote.
        Callback<void(FieldId)> onFieldChanged;
        Callback<void(ActionId)> onActionTriggered;
        Callback<void()> onTableChanged;

    private:
        [[nodiscard]] bool Holds(const Condition& condition) const;
        [[nodiscard]] std::size_t IndexOf(FieldId id) const;

        const FormSpec* spec;
        std::span<FieldValue> values;
        std::span<TableModel> tables;
    };
}
