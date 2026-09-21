#include "ui/scope/ScopeController.hpp"

namespace ui::scope
{
    ScopeController::ScopeController(ScopeCore& scope, model::FormModel& model, shell::FormView& view)
        : scope(&scope)
        , model(&model)
        , view(&view)
    {
        model.onFieldChanged = [this](model::FieldId changed)
        {
            OnFieldChanged(changed);
        };

        model.onActionTriggered = [this](model::ActionId triggered)
        {
            OnActionTriggered(triggered);
        };

        Refresh();
    }

    ScopeController::~ScopeController()
    {
        model->onFieldChanged.Reset();
        model->onActionTriggered.Reset();
    }

    void ScopeController::Refresh()
    {
        scope->SetTimePerDivision(SecondsPerDivisionAt(model->Selection(field::timePerDivision)));
        scope->SetTriggerMode(static_cast<TriggerMode>(model->SelectedData(field::triggerMode)));
        scope->SetTriggerEdge(static_cast<TriggerEdge>(model->SelectedData(field::triggerEdge)));
        scope->SetTriggerLevel(static_cast<float>(model->Number(field::triggerLevel)));
        scope->SetTriggerChannel(model->Selection(field::triggerChannel));

        ApplyRunState();
    }

    void ScopeController::OnFieldChanged(model::FieldId changed)
    {
        if (changed == field::timePerDivision)
            scope->SetTimePerDivision(SecondsPerDivisionAt(model->Selection(field::timePerDivision)));
        else if (changed == field::triggerMode)
            scope->SetTriggerMode(static_cast<TriggerMode>(model->SelectedData(field::triggerMode)));
        else if (changed == field::triggerEdge)
            scope->SetTriggerEdge(static_cast<TriggerEdge>(model->SelectedData(field::triggerEdge)));
        else if (changed == field::triggerLevel)
            scope->SetTriggerLevel(static_cast<float>(model->Number(field::triggerLevel)));
        else if (changed == field::triggerChannel)
            scope->SetTriggerChannel(model->Selection(field::triggerChannel));
    }

    void ScopeController::OnActionTriggered(model::ActionId triggered)
    {
        if (triggered == field::runStop)
        {
            scope->SetRunning(!scope->IsRunning());
            ApplyRunState();
        }
        else if (triggered == field::single)
        {
            model->SetSelection(field::triggerMode, static_cast<std::size_t>(TriggerMode::Single));
            view->Refresh(field::triggerMode);
            scope->SetTriggerMode(TriggerMode::Single);
            scope->SetRunning(true);
            ApplyRunState();
        }
        else if (triggered == field::force)
        {
            scope->ForceTrigger();
        }
    }

    void ScopeController::ApplyRunState()
    {
        const auto running = scope->IsRunning();
        const model::ActionSpec spec{ field::runStop, running ? stopLabel : runLabel,
            running ? theme::ButtonRole::Stop : theme::ButtonRole::Start, 0 };

        view->SetAction(field::runStop, spec);
    }
}
