#pragma once

#include "ui/model/FormModel.hpp"
#include "ui/theme/Theme.hpp"
#include <string_view>

namespace ui::shell
{
    // The seam between a form's description and the controls that realise it. A backend owns the
    // controls; the model owns the values; neither knows the other's type.
    class FormView
    {
    public:
        FormView() = default;
        FormView(const FormView& other) = delete;
        FormView& operator=(const FormView& other) = delete;

        // Realises the spec and installs the initial visibility and enablement. Calling it a second
        // time rebuilds from scratch.
        virtual void Build(model::FormModel& model) = 0;

        // Pushes model values into the controls without reporting them back as edits. A write from
        // elsewhere in the application goes through here.
        virtual void Refresh() = 0;
        virtual void Refresh(model::FieldId field) = 0;

        virtual void SetActionEnabled(model::ActionId action, bool enabled) = 0;

        virtual void SetAction(model::ActionId action, const model::ActionSpec& spec) = 0;

        // Marks one field with a status and a sentence that says why, such as a value that reads back
        // different or differs from a preset. Neutral with an empty note clears it; a rebuild clears all.
        // The note always travels with the colour, so the colour is never the only signal.
        virtual void SetFieldStatus(model::FieldId field, theme::StatusLevel level, std::string_view note) = 0;

        // These exist so the conformance suite can put the same question to every implementation.
        [[nodiscard]] virtual bool IsControlVisible(model::FieldId field) const = 0;
        [[nodiscard]] virtual bool IsControlEnabled(model::FieldId field) const = 0;
        [[nodiscard]] virtual theme::StatusLevel FieldStatus(model::FieldId field) const = 0;

    protected:
        ~FormView() = default;
    };
}
