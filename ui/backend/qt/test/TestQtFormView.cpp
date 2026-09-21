#include "ui/backend/qt/QtFormView.hpp"
#include "ui/backend/qt/test/FormTestSpec.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QSpinBox>
#include <gmock/gmock.h>

namespace
{
    using ui::backend::qt::QtFormView;

    class QtFormViewTest
        : public ::testing::Test
    {
    protected:
        QtFormViewTest()
        {
            ui::theme::SetCurrent(ui::theme::Light());
            view.Build(harness.Model());
        }

        template<class Widget>
        [[nodiscard]] Widget* ControlAs(ui::model::FieldId field) const
        {
            return qobject_cast<Widget*>(view.ControlFor(field));
        }

        formspec::Harness harness;
        QtFormView view;
    };
}

TEST_F(QtFormViewTest, EachFieldKindBecomesItsOwnControlType)
{
    EXPECT_NE(ControlAs<QDoubleSpinBox>(formspec::cutoff), nullptr);
    EXPECT_NE(ControlAs<QSpinBox>(formspec::order), nullptr);
    EXPECT_NE(ControlAs<QComboBox>(formspec::filterType), nullptr);
    EXPECT_NE(ControlAs<QCheckBox>(formspec::normalise), nullptr);
    EXPECT_NE(ControlAs<QLabel>(formspec::gainReadOut), nullptr);
}

TEST_F(QtFormViewTest, ANumberTranscribesItsRangeStepDecimalsAndSuffix)
{
    auto* editor = ControlAs<QDoubleSpinBox>(formspec::cutoff);
    ASSERT_NE(editor, nullptr);

    EXPECT_NEAR(editor->minimum(), 1.0, 1e-9);
    EXPECT_NEAR(editor->maximum(), 22050.0, 1e-9);
    EXPECT_NEAR(editor->singleStep(), 10.0, 1e-9);
    EXPECT_EQ(editor->decimals(), 1);
    EXPECT_EQ(editor->suffix(), " Hz");
    EXPECT_NEAR(editor->value(), 1000.0, 1e-9);
}

TEST_F(QtFormViewTest, AnIntegerFieldBecomesAnIntegerSpinner)
{
    auto* editor = ControlAs<QSpinBox>(formspec::order);
    ASSERT_NE(editor, nullptr);

    EXPECT_EQ(editor->minimum(), 3);
    EXPECT_EQ(editor->maximum(), 127);
    EXPECT_EQ(editor->singleStep(), 2);
    EXPECT_EQ(editor->value(), 31);
}

TEST_F(QtFormViewTest, AChoiceCarriesItsOptionDataRatherThanItsIndex)
{
    auto* editor = ControlAs<QComboBox>(formspec::filterType);
    ASSERT_NE(editor, nullptr);

    ASSERT_EQ(editor->count(), 3);
    EXPECT_EQ(editor->itemData(2).toLongLong(), 30);
}

// Every field label reaches Qt as UTF-8 from a literal, so a multi-byte glyph surviving intact is
// what proves the path end to end - including under a compiler that needs telling the source
// encoding.
TEST_F(QtFormViewTest, AMultiByteLabelSurvivesTheCrossingIntoQt)
{
    auto* editor = ControlAs<QDoubleSpinBox>(formspec::cutoff);
    ASSERT_NE(editor, nullptr);

    // Ten characters but eleven bytes: lambda is two bytes in UTF-8, and a compiler reading the
    // source in the wrong code page turns it into two separate characters rather than one.
    const auto expected = QString::fromUtf8("Cutoff (\xCE\xBB)");
    EXPECT_EQ(expected.size(), 10);
    EXPECT_EQ(std::string_view{ "Cutoff (\xCE\xBB)" }.size(), 11u);

    const auto* label = view.findChild<QLabel*>();
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->text(), expected);
}

TEST_F(QtFormViewTest, EditingAControlReachesTheModel)
{
    auto* editor = ControlAs<QDoubleSpinBox>(formspec::cutoff);
    ASSERT_NE(editor, nullptr);

    editor->setValue(2500.0);

    EXPECT_NEAR(harness.Model().Float(formspec::cutoff), 2500.0f, 1e-4f);
}

TEST_F(QtFormViewTest, PickingAnOptionReachesTheModel)
{
    auto* editor = ControlAs<QComboBox>(formspec::filterType);
    ASSERT_NE(editor, nullptr);

    editor->setCurrentIndex(2);

    EXPECT_EQ(harness.Model().SelectedData(formspec::filterType), 30);
}

TEST_F(QtFormViewTest, TogglingACheckBoxReachesTheModel)
{
    auto* editor = ControlAs<QCheckBox>(formspec::normalise);
    ASSERT_NE(editor, nullptr);

    editor->setChecked(true);

    EXPECT_TRUE(harness.Model().Flag(formspec::normalise));
}

// A write from elsewhere in the application has to move the control without coming back as a user
// edit; without the guard this is an endless round trip.
TEST_F(QtFormViewTest, RefreshMovesTheControlWithoutReportingAnEdit)
{
    auto changes = 0;
    harness.Model().onFieldChanged = [&changes](ui::model::FieldId)
    {
        ++changes;
    };

    harness.Model().SetNumber(formspec::cutoff, 4321.0);
    view.Refresh(formspec::cutoff);

    auto* editor = ControlAs<QDoubleSpinBox>(formspec::cutoff);
    ASSERT_NE(editor, nullptr);
    EXPECT_NEAR(editor->value(), 4321.0, 1e-9);
    EXPECT_EQ(changes, 1);
}

TEST_F(QtFormViewTest, AConditionalControlStartsOutOfTheStateItsChoiceForbids)
{
    EXPECT_FALSE(view.IsControlEnabled(formspec::cutoffHigh));
    EXPECT_TRUE(view.IsControlEnabled(formspec::cutoff));
}

TEST_F(QtFormViewTest, FlippingTheChoiceEnablesTheDependentControl)
{
    ControlAs<QComboBox>(formspec::filterType)->setCurrentIndex(2);

    EXPECT_TRUE(view.IsControlEnabled(formspec::cutoffHigh));
}

TEST_F(QtFormViewTest, AReadOutRendersItsValueWithItsDecimalsAndSuffix)
{
    harness.Model().SetSelection(formspec::filterType, 2);
    harness.Model().SetNumber(formspec::gainReadOut, 12.5);
    view.Refresh(formspec::gainReadOut);

    auto* readOut = ControlAs<QLabel>(formspec::gainReadOut);
    ASSERT_NE(readOut, nullptr);
    EXPECT_EQ(readOut->text(), "12.50 dB");
}

TEST_F(QtFormViewTest, AnActionBecomesAButtonThatCanBeDisabled)
{
    auto* button = view.ButtonFor(formspec::compute);
    ASSERT_NE(button, nullptr);
    EXPECT_TRUE(button->isEnabled());

    view.SetActionEnabled(formspec::compute, false);

    EXPECT_FALSE(button->isEnabled());
}

TEST_F(QtFormViewTest, PressingTheActionReachesItsSubscriber)
{
    ui::model::ActionId triggered{ ui::model::noAction };
    harness.Model().onActionTriggered = [&triggered](ui::model::ActionId id)
    {
        triggered = id;
    };

    view.ButtonFor(formspec::compute)->click();

    EXPECT_EQ(triggered, formspec::compute);
}

// The model outlives the widget in every consumer, so the callbacks have to come off with it.
TEST_F(QtFormViewTest, DestroyingTheViewClearsTheModelsCallbacks)
{
    formspec::Harness local;

    {
        QtFormView scoped;
        scoped.Build(local.Model());
        EXPECT_TRUE(static_cast<bool>(local.Model().onFieldChanged));
    }

    EXPECT_FALSE(static_cast<bool>(local.Model().onFieldChanged));
}
