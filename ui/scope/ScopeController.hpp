#pragma once

#include "ui/scope/ScopeControls.hpp"
#include "ui/scope/ScopeCore.hpp"
#include "ui/shell/FormView.hpp"

namespace ui::scope
{
    class ScopeController
    {
    public:
        ScopeController(ScopeCore& scope, model::FormModel& model, shell::FormView& view);
        ScopeController(const ScopeController& other) = delete;
        ScopeController& operator=(const ScopeController& other) = delete;
        ~ScopeController();

        void Refresh();

    private:
        void OnFieldChanged(model::FieldId changed);
        void OnActionTriggered(model::ActionId triggered);
        void ApplyRunState();

        ScopeCore* scope;
        model::FormModel* model;
        shell::FormView* view;
    };
}
