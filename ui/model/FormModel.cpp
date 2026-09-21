#include "ui/model/FormModel.hpp"
#include <algorithm>
#include <cmath>

namespace ui::model
{
    namespace
    {
        constexpr std::size_t notFound{ static_cast<std::size_t>(-1) };
    }

    FormModel::FormModel(const FormSpec& spec, std::span<FieldValue> values, std::span<TableModel> tables)
        : spec(&spec)
        , values(values)
        , tables(tables)
    {
        ResetToDefaults();
    }

    const FormSpec& FormModel::Spec() const
    {
        return *spec;
    }

    std::size_t FormModel::IndexOf(FieldId id) const
    {
        for (std::size_t i = 0; i < spec->fields.size(); ++i)
            if (spec->fields[i].id == id)
                return i;

        return notFound;
    }

    const FieldSpec& FormModel::Field(FieldId id) const
    {
        static constexpr FieldSpec missing{};

        const auto index = IndexOf(id);
        return index == notFound ? missing : spec->fields[index];
    }

    double FormModel::Number(FieldId id) const
    {
        const auto index = IndexOf(id);
        return index == notFound || index >= values.size() ? 0.0 : values[index].number;
    }

    float FormModel::Float(FieldId id) const
    {
        return static_cast<float>(Number(id));
    }

    std::int64_t FormModel::Integer(FieldId id) const
    {
        return std::llround(Number(id));
    }

    // The consumer panels expressed integers two ways - a genuine integer spinner, and a double
    // spinner with zero decimals read back through a cast. Both land here so a caller never has to
    // know which one the spec used.
    std::size_t FormModel::Count(FieldId id) const
    {
        const auto rounded = Integer(id);
        return rounded <= 0 ? 0u : static_cast<std::size_t>(rounded);
    }

    std::size_t FormModel::Selection(FieldId id) const
    {
        const auto index = IndexOf(id);
        return index == notFound || index >= values.size() ? 0u : values[index].selection;
    }

    std::int64_t FormModel::SelectedData(FieldId id) const
    {
        const auto& field = Field(id);
        const auto selection = Selection(id);

        if (selection >= field.options.size())
            return -1;

        const auto& option = field.options[selection];
        return option.data < 0 ? static_cast<std::int64_t>(selection) : option.data;
    }

    bool FormModel::Flag(FieldId id) const
    {
        const auto index = IndexOf(id);
        return index != notFound && index < values.size() && values[index].flag;
    }

    void FormModel::SetNumber(FieldId id, double value)
    {
        const auto index = IndexOf(id);
        if (index == notFound || index >= values.size())
            return;

        values[index].number = value;

        if (onFieldChanged)
            onFieldChanged(id);
    }

    void FormModel::SetSelection(FieldId id, std::size_t index)
    {
        const auto position = IndexOf(id);
        if (position == notFound || position >= values.size())
            return;

        if (index >= spec->fields[position].options.size())
            return;

        values[position].selection = static_cast<std::uint16_t>(index);

        if (onFieldChanged)
            onFieldChanged(id);
    }

    bool FormModel::SelectByData(FieldId id, std::int64_t data)
    {
        const auto& field = Field(id);

        for (std::size_t i = 0; i < field.options.size(); ++i)
        {
            const auto candidate = field.options[i].data < 0 ? static_cast<std::int64_t>(i) : field.options[i].data;

            if (candidate == data)
            {
                SetSelection(id, i);
                return true;
            }
        }

        return false;
    }

    void FormModel::SetFlag(FieldId id, bool value)
    {
        const auto index = IndexOf(id);
        if (index == notFound || index >= values.size())
            return;

        values[index].flag = value;

        if (onFieldChanged)
            onFieldChanged(id);
    }

    bool FormModel::Holds(const Condition& condition) const
    {
        if (condition.Unconditional())
            return true;

        const auto selection = Selection(condition.field);
        if (selection >= 32u)
            return false;

        return (condition.selectionMask & (1u << selection)) != 0u;
    }

    bool FormModel::IsVisible(FieldId id) const
    {
        const auto& field = Field(id);
        return Holds(field.visibleWhen) && IsGroupVisible(field.group);
    }

    bool FormModel::IsEnabled(FieldId id) const
    {
        return Holds(Field(id).enabledWhen);
    }

    bool FormModel::IsGroupVisible(GroupId id) const
    {
        if (id == noGroup)
            return true;

        for (const auto& group : spec->groups)
            if (group.id == id)
                return Holds(group.visibleWhen);

        return true;
    }

    std::size_t FormModel::TableCount() const
    {
        return tables.size();
    }

    TableModel& FormModel::Table(std::size_t index)
    {
        return tables[index];
    }

    const TableModel& FormModel::Table(std::size_t index) const
    {
        return tables[index];
    }

    void FormModel::ResetToDefaults()
    {
        const auto count = std::min(spec->fields.size(), values.size());

        for (std::size_t i = 0; i < count; ++i)
        {
            values[i].number = spec->fields[i].number.initial;
            values[i].selection = 0;
            values[i].flag = false;
        }
    }

    // Range violations are unreachable through a backend that clamps its own controls; this exists
    // for values written programmatically and for a backend that does not clamp.
    std::optional<Violation> FormModel::Validate() const
    {
        const auto count = std::min(spec->fields.size(), values.size());

        for (std::size_t i = 0; i < count; ++i)
        {
            const auto& field = spec->fields[i];

            if (!Holds(field.visibleWhen) || !IsGroupVisible(field.group))
                continue;

            if (field.kind == FieldKind::Number || field.kind == FieldKind::Integer)
            {
                if (values[i].number < field.number.minimum)
                    return Violation{ field.id, ViolationKind::BelowMinimum };

                if (values[i].number > field.number.maximum)
                    return Violation{ field.id, ViolationKind::AboveMaximum };
            }

            if (field.kind == FieldKind::Choice && field.options.empty())
                return Violation{ field.id, ViolationKind::NoSelection };
        }

        return std::nullopt;
    }

    void FormModel::TriggerAction(ActionId id)
    {
        if (onActionTriggered)
            onActionTriggered(id);
    }

    void FormModel::NotifyTableChanged()
    {
        if (onTableChanged)
            onTableChanged();
    }
}
