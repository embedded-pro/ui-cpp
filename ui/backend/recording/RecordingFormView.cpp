#include "ui/backend/recording/RecordingFormView.hpp"
#include <algorithm>
#include <ranges>

namespace ui::backend::recording
{
    namespace
    {
        FormCommandKind CreateKindFor(model::FieldKind kind)
        {
            using enum model::FieldKind;
            using enum FormCommandKind;

            switch (kind)
            {
                case Integer:
                    return CreateInteger;
                case Slider:
                    return CreateSlider;
                case Choice:
                    return CreateChoice;
                case Toggle:
                    return CreateToggle;
                case ReadOut:
                    return CreateReadOut;
                case Number:
                default:
                    return CreateNumber;
            }
        }
    }

    FormCommand& RecordingFormView::Append(FormCommandKind kind)
    {
        auto& command = commands.emplace_back();
        command.kind = kind;

        return command;
    }

    void RecordingFormView::Build(model::FormModel& formModel)
    {
        model = &formModel;
        commands.clear();
        fieldOrder.clear();

        for (const auto& group : formModel.Spec().groups)
        {
            auto& begin = Append(FormCommandKind::BeginGroup);
            begin.group = group.id;
            begin.label = std::string{ group.title };
        }

        for (const auto& field : formModel.Spec().fields)
        {
            auto& command = Append(CreateKindFor(field.kind));
            command.field = field.id;
            command.group = field.group;
            command.label = std::string{ field.label };
            command.suffix = std::string{ field.suffix };
            command.number = field.number.initial;
            command.minimum = field.number.minimum;
            command.maximum = field.number.maximum;
            command.step = field.number.step;
            command.tickInterval = field.number.tickInterval;
            command.decimals = field.number.decimals;
            command.count = field.options.size();

            fieldOrder.push_back(field.id);
        }

        for (const auto& action : formModel.Spec().actions)
        {
            auto& command = Append(FormCommandKind::CreateAction);
            command.action = action.id;
            command.label = std::string{ action.label };
            command.buttonRole = action.buttonRole;
        }

        for (std::size_t i = 0; i < formModel.Spec().tables.size(); ++i)
        {
            auto& command = Append(FormCommandKind::CreateTable);
            command.index = i;
            command.count = formModel.Spec().tables[i].columns.size();
        }

        visible.assign(fieldOrder.size(), true);
        enabled.assign(fieldOrder.size(), true);

        formModel.onFieldChanged = [this](model::FieldId field)
        {
            OnFieldChanged(field);
        };

        // Running this at build time rather than only on change is what makes a form's initial
        // enablement correct; the panels this replaces connected their handlers after setting the
        // initial selection and so never synchronised it.
        ApplyConditions(true);
    }

    void RecordingFormView::OnFieldChanged(model::FieldId field)
    {
        static_cast<void>(field);

        ++fieldChangeCount;
        ApplyConditions(false);
    }

    // force is set once, from Build: every field states its initial visibility and enablement even
    // when that matches the default, so a test can assert the starting state exists rather than
    // infer it from silence. Afterwards only changes are recorded.
    void RecordingFormView::ApplyConditions(bool force)
    {
        if (model == nullptr)
            return;

        for (std::size_t i = 0; i < fieldOrder.size(); ++i)
        {
            const auto isVisible = model->IsVisible(fieldOrder[i]);
            const auto isEnabled = model->IsEnabled(fieldOrder[i]);

            if (force || visible[i] != isVisible)
            {
                auto& command = Append(FormCommandKind::SetVisible);
                command.field = fieldOrder[i];
                command.flag = isVisible;
            }

            if (force || enabled[i] != isEnabled)
            {
                auto& command = Append(FormCommandKind::SetEnabled);
                command.field = fieldOrder[i];
                command.flag = isEnabled;
            }

            visible[i] = isVisible;
            enabled[i] = isEnabled;
        }
    }

    void RecordingFormView::Refresh()
    {
        if (model == nullptr)
            return;

        applying = true;

        for (const auto id : fieldOrder)
            Refresh(id);

        applying = false;
    }

    void RecordingFormView::Refresh(model::FieldId field)
    {
        if (model == nullptr)
            return;

        const auto& spec = model->Field(field);

        switch (spec.kind)
        {
            case model::FieldKind::Choice:
            {
                auto& command = Append(FormCommandKind::SetSelection);
                command.field = field;
                command.index = model->Selection(field);
                break;
            }
            case model::FieldKind::Toggle:
            {
                auto& command = Append(FormCommandKind::SetFlag);
                command.field = field;
                command.flag = model->Flag(field);
                break;
            }
            default:
            {
                auto& command = Append(FormCommandKind::SetValue);
                command.field = field;
                command.number = model->Number(field);
                break;
            }
        }
    }

    void RecordingFormView::SetActionEnabled(model::ActionId action, bool isEnabled)
    {
        auto& command = Append(FormCommandKind::SetActionEnabled);
        command.action = action;
        command.flag = isEnabled;
    }

    void RecordingFormView::SetAction(model::ActionId action, const model::ActionSpec& spec)
    {
        auto& command = Append(FormCommandKind::SetActionSpec);
        command.action = action;
        command.label = std::string{ spec.label };
        command.buttonRole = spec.buttonRole;
        command.number = static_cast<double>(spec.minimumHeight);
    }

    bool RecordingFormView::IsControlVisible(model::FieldId field) const
    {
        for (std::size_t i = 0; i < fieldOrder.size(); ++i)
            if (fieldOrder[i] == field)
                return visible[i];

        return false;
    }

    bool RecordingFormView::IsControlEnabled(model::FieldId field) const
    {
        for (std::size_t i = 0; i < fieldOrder.size(); ++i)
            if (fieldOrder[i] == field)
                return enabled[i];

        return false;
    }

    void RecordingFormView::TypeNumber(model::FieldId field, double value)
    {
        if (model != nullptr && !applying)
            model->SetNumber(field, value);
    }

    void RecordingFormView::PickOption(model::FieldId field, std::size_t index)
    {
        if (model != nullptr && !applying)
            model->SetSelection(field, index);
    }

    void RecordingFormView::ToggleFlag(model::FieldId field, bool value)
    {
        if (model != nullptr && !applying)
            model->SetFlag(field, value);
    }

    void RecordingFormView::PressAction(model::ActionId action)
    {
        if (model != nullptr)
            model->TriggerAction(action);
    }

    bool RecordingFormView::PressAddRow(std::size_t table)
    {
        if (model == nullptr || table >= model->TableCount())
            return false;

        const auto added = model->Table(table).AddRow();
        if (added)
            model->NotifyTableChanged();

        return added;
    }

    bool RecordingFormView::PressRemoveRow(std::size_t table, std::size_t row)
    {
        if (model == nullptr || table >= model->TableCount())
            return false;

        const auto removed = model->Table(table).RemoveRow(row);
        if (removed)
            model->NotifyTableChanged();

        return removed;
    }

    const std::vector<FormCommand>& RecordingFormView::Commands() const
    {
        return commands;
    }

    std::size_t RecordingFormView::CountOf(FormCommandKind kind) const
    {
        return static_cast<std::size_t>(std::ranges::count_if(commands,
            [kind](const FormCommand& command)
            {
                return command.kind == kind;
            }));
    }

    std::vector<std::string> RecordingFormView::Labels() const
    {
        std::vector<std::string> result;

        for (const auto& command : commands)
            if (!command.label.empty())
                result.push_back(command.label);

        return result;
    }

    std::size_t RecordingFormView::FieldChangeCount() const
    {
        return fieldChangeCount;
    }

    void RecordingFormView::Clear()
    {
        commands.clear();
        fieldChangeCount = 0;
    }
}
