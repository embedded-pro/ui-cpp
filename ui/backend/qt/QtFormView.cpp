#include "ui/backend/qt/QtFormView.hpp"
#include "ui/backend/qt/QtConversions.hpp"
#include "ui/backend/qt/QtFormTable.hpp"
#include "ui/backend/qt/QtTheme.hpp"
#include "ui/core/Format.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QVariant>

namespace ui::backend::qt
{
    QtFormView::QtFormView(QWidget* parent)
        : QWidget(parent)
        , outerLayout(new QVBoxLayout{ this })
        , rootLayout(new QFormLayout{})
    {
        outerLayout->setContentsMargins(0, 0, 0, 0);

        outerLayout->addLayout(rootLayout);
        outerLayout->addStretch();
    }

    QtFormView::~QtFormView()
    {
        // The model outlives this widget in every consumer, so leaving the callbacks installed
        // would call back into a destroyed QWidget.
        if (model != nullptr)
        {
            model->onFieldChanged.Reset();
            model->onTableChanged.Reset();
        }
    }

    void QtFormView::Clear()
    {
        controls.clear();
        actions.clear();
        tables.clear();
        groups.clear();
    }

    QFormLayout& QtFormView::LayoutFor(model::GroupId group)
    {
        if (group == model::noGroup)
            return *rootLayout;

        for (const auto& [id, box] : groups)
            if (id == group)
                return *static_cast<QFormLayout*>(box->layout());

        return *rootLayout;
    }

    void QtFormView::AddRow(model::GroupId group, QWidget* label, QWidget* editor)
    {
        if (group == model::noGroup && inlineLayout != nullptr)
        {
            if (label != nullptr)
                inlineLayout->addWidget(label);

            inlineLayout->addWidget(editor);
            return;
        }

        auto& layout = LayoutFor(group);

        if (label != nullptr)
            layout.addRow(label, editor);
        else
            layout.addRow(editor);
    }

    void QtFormView::CreateField(const model::FieldSpec& field)
    {
        Control control{};
        control.field = field.id;

        switch (field.kind)
        {
            case model::FieldKind::Number:
            {
                auto* editor = new QDoubleSpinBox{ this };
                editor->setRange(field.number.minimum, field.number.maximum);
                editor->setDecimals(field.number.decimals);
                editor->setSingleStep(field.number.step);
                editor->setValue(model->Number(field.id));

                if (!field.suffix.empty())
                    editor->setSuffix(ToQt(field.suffix));

                connect(editor, &QDoubleSpinBox::valueChanged, this, [this, id = field.id](double value)
                    {
                        if (!applying)
                            model->SetNumber(id, value);
                    });

                control.editor = editor;
                break;
            }
            case model::FieldKind::Integer:
            {
                auto* editor = new QSpinBox{ this };
                editor->setRange(static_cast<int>(field.number.minimum), static_cast<int>(field.number.maximum));
                editor->setSingleStep(static_cast<int>(field.number.step));
                editor->setValue(static_cast<int>(model->Number(field.id)));

                if (!field.suffix.empty())
                    editor->setSuffix(ToQt(field.suffix));

                connect(editor, &QSpinBox::valueChanged, this, [this, id = field.id](int value)
                    {
                        if (!applying)
                            model->SetNumber(id, static_cast<double>(value));
                    });

                control.editor = editor;
                break;
            }
            case model::FieldKind::Slider:
            {
                auto* editor = new QSlider{ ::Qt::Horizontal, this };
                editor->setRange(static_cast<int>(field.number.minimum), static_cast<int>(field.number.maximum));
                editor->setSingleStep(static_cast<int>(field.number.step));
                editor->setValue(static_cast<int>(model->Number(field.id)));

                if (field.number.tickInterval > 0.0)
                {
                    editor->setTickInterval(static_cast<int>(field.number.tickInterval));
                    editor->setTickPosition(QSlider::TicksBelow);
                }

                connect(editor, &QSlider::valueChanged, this, [this, id = field.id](int value)
                    {
                        if (!applying)
                            model->SetNumber(id, static_cast<double>(value));
                    });

                control.editor = editor;
                break;
            }
            case model::FieldKind::Choice:
            {
                auto* editor = new QComboBox{ this };

                for (std::size_t i = 0; i < field.options.size(); ++i)
                {
                    const auto& option = field.options[i];
                    const auto data = option.data < 0 ? static_cast<std::int64_t>(i) : option.data;
                    editor->addItem(ToQt(option.label), QVariant::fromValue(static_cast<qlonglong>(data)));
                }

                editor->setCurrentIndex(static_cast<int>(model->Selection(field.id)));

                connect(editor, &QComboBox::currentIndexChanged, this, [this, id = field.id](int index)
                    {
                        if (!applying && index >= 0)
                            model->SetSelection(id, static_cast<std::size_t>(index));
                    });

                control.editor = editor;
                break;
            }
            case model::FieldKind::Toggle:
            {
                auto* editor = new QCheckBox{ ToQt(field.label), this };
                editor->setChecked(model->Flag(field.id));

                connect(editor, &QCheckBox::toggled, this, [this, id = field.id](bool value)
                    {
                        if (!applying)
                            model->SetFlag(id, value);
                    });

                control.editor = editor;
                break;
            }
            case model::FieldKind::ReadOut:
            default:
            {
                auto* editor = new QLabel{ this };
                editor->setFont(ToQt(theme::Current().Get(theme::FontRole::Monospace)));
                control.editor = editor;
                break;
            }
        }

        // A toggle carries its own text, so giving it a second label beside the box would state the
        // same thing twice.
        if (field.kind != model::FieldKind::Toggle)
            control.label = new QLabel{ ToQt(field.label), this };

        AddRow(field.group, control.label, control.editor);

        controls.push_back(control);
    }

    void QtFormView::Build(model::FormModel& formModel)
    {
        model = &formModel;
        Clear();

        if (formModel.Spec().layout == model::FormLayout::Inline)
        {
            inlineLayout = new QHBoxLayout{};
            inlineLayout->setContentsMargins(0, 0, 0, 0);
            outerLayout->insertLayout(0, inlineLayout);

            // One row is exactly as tall as it needs to be. Without this the strip keeps the
            // vertical policy a stacked form wants and takes height from whatever it sits above.
            setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        }

        for (const auto& group : formModel.Spec().groups)
        {
            auto* box = new QGroupBox{ ToQt(group.title), this };
            new QFormLayout{ box };
            AddRow(model::noGroup, nullptr, box);
            groups.emplace_back(group.id, box);
        }

        for (const auto& field : formModel.Spec().fields)
            CreateField(field);

        for (std::size_t i = 0; i < formModel.TableCount(); ++i)
        {
            auto* table = new QtFormTable{ formModel, i, this };
            AddRow(formModel.Table(i).Spec().group, nullptr, table);
            tables.push_back(table);
        }

        for (const auto& action : formModel.Spec().actions)
        {
            auto* button = new QPushButton{ ToQt(action.label), this };
            StyleButton(*button, action.buttonRole);

            if (action.minimumHeight > 0)
                button->setMinimumHeight(action.minimumHeight);

            connect(button, &QPushButton::clicked, this, [this, id = action.id]
                {
                    model->TriggerAction(id);
                });

            AddRow(model::noGroup, nullptr, button);
            actions.push_back(Action{ action.id, button });
        }

        // Without it the row shares its spare width between the controls, stretching a combo box
        // across the panel; an instrument strip wants them at their natural size, packed left.
        if (inlineLayout != nullptr)
            inlineLayout->addStretch();

        formModel.onFieldChanged = [this](model::FieldId field)
        {
            OnFieldChanged(field);
        };

        // Applied here as well as on change: the panels this replaces connected their handlers
        // after setting the initial selection, so a conditional control started out of step with
        // the choice driving it.
        ApplyConditions();
    }

    void QtFormView::OnFieldChanged(model::FieldId field)
    {
        static_cast<void>(field);
        ApplyConditions();
    }

    void QtFormView::ApplyConditions()
    {
        for (const auto& control : controls)
        {
            const auto visible = model->IsVisible(control.field);
            const auto enabled = model->IsEnabled(control.field);

            control.editor->setVisible(visible);
            control.editor->setEnabled(enabled);

            if (control.label != nullptr)
            {
                control.label->setVisible(visible);
                control.label->setEnabled(enabled);
            }
        }

        for (const auto& [id, box] : groups)
            box->setVisible(model->IsGroupVisible(id));
    }

    void QtFormView::Refresh()
    {
        for (const auto& control : controls)
            Refresh(control.field);

        for (auto* table : tables)
            table->Refresh();
    }

    void QtFormView::Refresh(model::FieldId field)
    {
        const auto* control = Find(field);
        if (control == nullptr)
            return;

        applying = true;

        if (auto* number = qobject_cast<QDoubleSpinBox*>(control->editor))
            number->setValue(model->Number(field));
        else if (auto* integer = qobject_cast<QSpinBox*>(control->editor))
            integer->setValue(static_cast<int>(model->Number(field)));
        else if (auto* slider = qobject_cast<QSlider*>(control->editor))
            slider->setValue(static_cast<int>(model->Number(field)));
        else if (auto* choice = qobject_cast<QComboBox*>(control->editor))
            choice->setCurrentIndex(static_cast<int>(model->Selection(field)));
        else if (auto* toggle = qobject_cast<QCheckBox*>(control->editor))
            toggle->setChecked(model->Flag(field));
        else if (auto* readOut = qobject_cast<QLabel*>(control->editor))
        {
            FormatBuffer<48> buffer;
            const auto& spec = model->Field(field);
            const auto value = static_cast<float>(model->Number(field));
            const auto digits = spec.number.decimals;
            const auto text = spec.number.readOut == model::ReadOutStyle::Significant ? buffer.Significant(value, digits) : buffer.Fixed(value, digits);

            readOut->setText(ToQt(text) + ToQt(spec.suffix));
        }

        applying = false;
    }

    void QtFormView::SetActionEnabled(model::ActionId action, bool enabled)
    {
        for (const auto& candidate : actions)
            if (candidate.id == action)
                candidate.button->setEnabled(enabled);
    }

    void QtFormView::SetAction(model::ActionId action, const model::ActionSpec& spec)
    {
        auto* button = ButtonFor(action);

        if (button == nullptr)
            return;

        button->setText(ToQt(spec.label));
        StyleButton(*button, spec.buttonRole);

        if (spec.minimumHeight > 0)
            button->setMinimumHeight(spec.minimumHeight);
    }

    const QtFormView::Control* QtFormView::Find(model::FieldId field) const
    {
        for (const auto& control : controls)
            if (control.field == field)
                return &control;

        return nullptr;
    }

    bool QtFormView::IsControlVisible(model::FieldId field) const
    {
        const auto* control = Find(field);
        return control != nullptr && !control->editor->isHidden();
    }

    bool QtFormView::IsControlEnabled(model::FieldId field) const
    {
        const auto* control = Find(field);
        return control != nullptr && control->editor->isEnabled();
    }

    QWidget* QtFormView::ControlFor(model::FieldId field) const
    {
        const auto* control = Find(field);
        return control == nullptr ? nullptr : control->editor;
    }

    QAbstractButton* QtFormView::ButtonFor(model::ActionId action) const
    {
        for (const auto& candidate : actions)
            if (candidate.id == action)
                return candidate.button;

        return nullptr;
    }
}
