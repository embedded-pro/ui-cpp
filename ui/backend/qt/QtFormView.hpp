#pragma once

#include "ui/model/FormModel.hpp"
#include "ui/shell/FormView.hpp"
#include <QWidget>
#include <vector>

class QAbstractButton;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFormLayout;
class QGroupBox;
class QHBoxLayout;
class QLabel;
class QSpinBox;
class QVBoxLayout;

namespace ui::backend::qt
{
    class QtFormTable;

    // Realises a form spec as native controls. Holds the model by reference and clears its
    // callbacks on the way out, the same arrangement QtPaintedWidget uses for a view: the model
    // belongs to the application and may outlive the widget showing it.
    class QtFormView
        : public QWidget
        , public shell::FormView
    {
        Q_OBJECT

    public:
        explicit QtFormView(QWidget* parent = nullptr);
        ~QtFormView() override;

        void Build(model::FormModel& model) override;
        void Refresh() override;
        void Refresh(model::FieldId field) override;
        void SetActionEnabled(model::ActionId action, bool enabled) override;
        void SetAction(model::ActionId action, const model::ActionSpec& spec) override;

        [[nodiscard]] bool IsControlVisible(model::FieldId field) const override;
        [[nodiscard]] bool IsControlEnabled(model::FieldId field) const override;

        [[nodiscard]] QWidget* ControlFor(model::FieldId field) const;
        [[nodiscard]] QAbstractButton* ButtonFor(model::ActionId action) const;

    private:
        struct Control
        {
            model::FieldId field{ model::noField };
            QWidget* editor{ nullptr };
            QWidget* row{ nullptr };
            QLabel* label{ nullptr };
        };

        struct Action
        {
            model::ActionId id{ model::noAction };
            QAbstractButton* button{ nullptr };
        };

        void Clear();
        void CreateField(const model::FieldSpec& field);
        void AddRow(model::GroupId group, QWidget* label, QWidget* editor);
        [[nodiscard]] QFormLayout& LayoutFor(model::GroupId group);
        void ApplyConditions();
        void OnFieldChanged(model::FieldId field);
        [[nodiscard]] const Control* Find(model::FieldId field) const;

        model::FormModel* model{ nullptr };
        QVBoxLayout* outerLayout{ nullptr };
        QFormLayout* rootLayout{ nullptr };

        // Non-null only for FormLayout::Inline, where it replaces rootLayout as the destination for
        // ungrouped controls; a group keeps its own stacked rows either way.
        QHBoxLayout* inlineLayout{ nullptr };

        std::vector<Control> controls;
        std::vector<Action> actions;
        std::vector<QtFormTable*> tables;
        std::vector<std::pair<model::GroupId, QGroupBox*>> groups;

        // Qt's widget model is single-threaded, so one flag is enough to tell an edit the user made
        // from one this view is writing back. It is not a lock and must not be treated as one.
        bool applying{ false };
    };
}
