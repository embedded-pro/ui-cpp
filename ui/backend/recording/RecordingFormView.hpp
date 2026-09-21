#pragma once

#include "ui/model/FormModel.hpp"
#include "ui/shell/FormView.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace ui::backend::recording
{
    enum class FormCommandKind : std::uint8_t
    {
        BeginGroup,
        EndGroup,
        CreateNumber,
        CreateInteger,
        CreateChoice,
        CreateToggle,
        CreateReadOut,
        CreateAction,
        CreateTable,
        SetValue,
        SetSelection,
        SetFlag,
        SetVisible,
        SetEnabled,
        SetActionEnabled
    };

    struct FormCommand
    {
        FormCommandKind kind{};
        model::FieldId field{ model::noField };
        model::GroupId group{ model::noGroup };
        model::ActionId action{ model::noAction };
        std::string label;
        std::string suffix;
        double number{ 0.0 };
        double minimum{ 0.0 };
        double maximum{ 0.0 };
        double step{ 0.0 };
        std::uint8_t decimals{ 0 };
        std::size_t index{ 0 };
        std::size_t count{ 0 };
        bool flag{ false };
    };

    // The second FormView implementation. Its existence is what demonstrates the form model is not
    // secretly Qt-shaped, exactly as RecordingCanvas does for Canvas, and it is how a form is
    // tested with no Qt and no display - which is what the macOS and Windows jobs run.
    class RecordingFormView
        : public shell::FormView
    {
    public:
        void Build(model::FormModel& model) override;
        void Refresh() override;
        void Refresh(model::FieldId field) override;
        void SetActionEnabled(model::ActionId action, bool enabled) override;

        [[nodiscard]] bool IsControlVisible(model::FieldId field) const override;
        [[nodiscard]] bool IsControlEnabled(model::FieldId field) const override;

        // Drive the model the way a person would, so a test asserts a round trip rather than the
        // setter it just called.
        void TypeNumber(model::FieldId field, double value);
        void PickOption(model::FieldId field, std::size_t index);
        void ToggleFlag(model::FieldId field, bool value);
        void PressAction(model::ActionId action);
        bool PressAddRow(std::size_t table);
        bool PressRemoveRow(std::size_t table, std::size_t row);

        [[nodiscard]] const std::vector<FormCommand>& Commands() const;
        [[nodiscard]] std::size_t CountOf(FormCommandKind kind) const;
        [[nodiscard]] std::vector<std::string> Labels() const;
        [[nodiscard]] std::size_t FieldChangeCount() const;
        void Clear();

    private:
        FormCommand& Append(FormCommandKind kind);
        void ApplyConditions(bool force);
        void OnFieldChanged(model::FieldId field);

        model::FormModel* model{ nullptr };
        std::vector<FormCommand> commands;
        std::vector<model::FieldId> fieldOrder;
        std::vector<bool> visible;
        std::vector<bool> enabled;
        std::size_t fieldChangeCount{ 0 };
        bool applying{ false };
    };
}
