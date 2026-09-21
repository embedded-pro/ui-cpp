#include "ui/backend/qt/QtFormView.hpp"
#include "ui/backend/qt/test/FormTestSpec.hpp"
#include "ui/backend/recording/RecordingFormView.hpp"
#include <QComboBox>
#include <QDoubleSpinBox>
#include <gmock/gmock.h>

namespace
{
    using ui::backend::qt::QtFormView;
    using ui::backend::recording::FormCommandKind;
    using ui::backend::recording::RecordingFormView;

    // The gate the form abstraction rests on. Both implementations are driven through the same
    // spec and the same gestures, and asked the same questions: if a Qt-only notion ever reaches
    // FieldSpec, one side stops being able to answer and this is what fails.
    class QtFormConformanceTest
        : public ::testing::Test
    {
    protected:
        QtFormConformanceTest()
        {
            ui::theme::SetCurrent(ui::theme::Light());
            native.Build(nativeHarness.Model());
            recorded.Build(recordedHarness.Model());
        }

        [[nodiscard]] QComboBox* NativeChoice() const
        {
            return qobject_cast<QComboBox*>(native.ControlFor(formspec::filterType));
        }

        formspec::Harness nativeHarness;
        formspec::Harness recordedHarness;
        QtFormView native;
        RecordingFormView recorded;
    };
}

TEST_F(QtFormConformanceTest, BothRealiseTheSameNumberOfControls)
{
    auto nativeNumbers = 0;
    for (const auto field : { formspec::cutoff, formspec::cutoffHigh })
        if (native.ControlFor(field) != nullptr)
            ++nativeNumbers;

    EXPECT_EQ(static_cast<std::size_t>(nativeNumbers), recorded.CountOf(FormCommandKind::CreateNumber));
    EXPECT_EQ(recorded.CountOf(FormCommandKind::CreateInteger), 1u);
    EXPECT_EQ(recorded.CountOf(FormCommandKind::CreateChoice), 1u);
    EXPECT_NE(native.ControlFor(formspec::order), nullptr);
    EXPECT_NE(native.ControlFor(formspec::filterType), nullptr);
}

TEST_F(QtFormConformanceTest, BothAgreeOnTheInitialConditionState)
{
    for (const auto field : { formspec::cutoff, formspec::order, formspec::filterType, formspec::cutoffHigh, formspec::normalise })
    {
        EXPECT_EQ(native.IsControlEnabled(field), recorded.IsControlEnabled(field)) << "field " << field.value;
        EXPECT_EQ(native.IsControlVisible(field), recorded.IsControlVisible(field)) << "field " << field.value;
    }
}

TEST_F(QtFormConformanceTest, BothAgreeAfterTheSameConditionFlip)
{
    NativeChoice()->setCurrentIndex(2);
    recorded.PickOption(formspec::filterType, 2);

    EXPECT_EQ(native.IsControlEnabled(formspec::cutoffHigh), recorded.IsControlEnabled(formspec::cutoffHigh));
    EXPECT_TRUE(native.IsControlEnabled(formspec::cutoffHigh));

    NativeChoice()->setCurrentIndex(0);
    recorded.PickOption(formspec::filterType, 0);

    EXPECT_EQ(native.IsControlEnabled(formspec::cutoffHigh), recorded.IsControlEnabled(formspec::cutoffHigh));
    EXPECT_FALSE(native.IsControlEnabled(formspec::cutoffHigh));
}

TEST_F(QtFormConformanceTest, BothAgreeOnGroupVisibility)
{
    EXPECT_EQ(native.IsControlVisible(formspec::gainReadOut), recorded.IsControlVisible(formspec::gainReadOut));

    NativeChoice()->setCurrentIndex(2);
    recorded.PickOption(formspec::filterType, 2);

    EXPECT_EQ(native.IsControlVisible(formspec::gainReadOut), recorded.IsControlVisible(formspec::gainReadOut));
}

TEST_F(QtFormConformanceTest, TheSameGestureLeavesBothModelsEqual)
{
    qobject_cast<QDoubleSpinBox*>(native.ControlFor(formspec::cutoff))->setValue(2500.0);
    recorded.TypeNumber(formspec::cutoff, 2500.0);

    NativeChoice()->setCurrentIndex(1);
    recorded.PickOption(formspec::filterType, 1);

    EXPECT_NEAR(nativeHarness.Model().Float(formspec::cutoff), recordedHarness.Model().Float(formspec::cutoff), 1e-4f);
    EXPECT_EQ(nativeHarness.Model().SelectedData(formspec::filterType), recordedHarness.Model().SelectedData(formspec::filterType));
}

TEST_F(QtFormConformanceTest, BothClampTheTableToTheSameCapacity)
{
    while (recorded.PressAddRow(0))
    {
    }

    EXPECT_EQ(recordedHarness.Model().Table(0).RowCount(), recordedHarness.Model().Table(0).MaximumRows());
    EXPECT_FALSE(recorded.PressAddRow(0));
}
