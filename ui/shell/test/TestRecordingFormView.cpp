#include "ui/backend/recording/RecordingFormView.hpp"
#include <array>
#include <gmock/gmock.h>

namespace
{
    using ui::backend::recording::FormCommandKind;
    using ui::backend::recording::RecordingFormView;
    using ui::model::ActionId;
    using ui::model::ColumnSpec;
    using ui::model::Condition;
    using ui::model::FieldId;
    using ui::model::FieldKind;
    using ui::model::FieldSpec;
    using ui::model::FieldValue;
    using ui::model::FormModel;
    using ui::model::FormSpec;
    using ui::model::OptionSpec;
    using ui::model::TableModel;
    using ui::model::TableSpec;

    constexpr FieldId cutoff{ 1 };
    constexpr FieldId order{ 2 };
    constexpr FieldId filterType{ 3 };
    constexpr FieldId cutoffHigh{ 4 };
    constexpr FieldId normalise{ 5 };
    constexpr ActionId compute{ 1 };

    constexpr std::array<OptionSpec, 3> filterOptions{
        OptionSpec{ "Low-Pass", 10 },
        OptionSpec{ "High-Pass", 20 },
        OptionSpec{ "Band-Pass", 30 }
    };

    constexpr std::array<FieldSpec, 5> fields{
        FieldSpec{ cutoff, ui::model::noGroup, FieldKind::Number, "Cutoff", " Hz", { 1.0, 22050.0, 10.0, 1000.0, 1 }, {}, {}, {} },
        FieldSpec{ order, ui::model::noGroup, FieldKind::Integer, "Order", "", { 3.0, 127.0, 2.0, 31.0, 0 }, {}, {}, {} },
        FieldSpec{ filterType, ui::model::noGroup, FieldKind::Choice, "Type", "", {}, filterOptions, {}, {} },
        FieldSpec{ cutoffHigh, ui::model::noGroup, FieldKind::Number, "Cutoff High", " Hz", { 1.0, 22050.0, 10.0, 4000.0, 1 }, {}, {}, Condition{ filterType, 0b100u } },
        FieldSpec{ normalise, ui::model::noGroup, FieldKind::Toggle, "Normalise", "", {}, {}, {}, {} }
    };

    constexpr std::array<ui::model::ActionSpec, 1> actions{
        ui::model::ActionSpec{ compute, "Compute", ui::theme::ButtonRole::Primary, 40 }
    };

    constexpr std::array<ColumnSpec, 2> columns{
        ColumnSpec{ "Frequency (Hz)", { 0.0, 96000.0, 10.0, 1000.0, 1 } },
        ColumnSpec{ "Amplitude", { 0.0, 1.0, 0.01, 0.5, 3 } }
    };

    class RecordingFormViewTest
        : public ::testing::Test
    {
    protected:
        RecordingFormViewTest()
        {
            view.Build(model);
        }

        [[nodiscard]] const ui::backend::recording::FormCommand* LastOf(FormCommandKind kind) const
        {
            const ui::backend::recording::FormCommand* found = nullptr;

            for (const auto& command : view.Commands())
                if (command.kind == kind)
                    found = &command;

            return found;
        }

        TableSpec tableSpec{ ui::model::noGroup, columns, 4, "Add", "Remove" };
        std::array<TableSpec, 1> tableSpecs{ tableSpec };
        std::array<double, 8> cells{};
        std::array<TableModel, 1> tables{ TableModel{ tableSpecs[0], cells } };

        std::array<FieldValue, 5> storage{};
        FormSpec spec{ {}, fields, actions, tableSpecs };
        FormModel model{ spec, storage, tables };
        RecordingFormView view;
    };
}

TEST_F(RecordingFormViewTest, BuildRealisesEveryFieldKindOnce)
{
    EXPECT_EQ(view.CountOf(FormCommandKind::CreateNumber), 2u);
    EXPECT_EQ(view.CountOf(FormCommandKind::CreateInteger), 1u);
    EXPECT_EQ(view.CountOf(FormCommandKind::CreateChoice), 1u);
    EXPECT_EQ(view.CountOf(FormCommandKind::CreateAction), 1u);
    EXPECT_EQ(view.CountOf(FormCommandKind::CreateTable), 1u);
}

TEST_F(RecordingFormViewTest, ANumberCarriesItsRangeStepDecimalsAndSuffix)
{
    const auto& commands = view.Commands();
    const auto found = std::find_if(commands.begin(), commands.end(),
        [](const ui::backend::recording::FormCommand& command)
        {
            return command.kind == FormCommandKind::CreateNumber && command.field == cutoff;
        });

    ASSERT_NE(found, commands.end());
    EXPECT_EQ(found->label, "Cutoff");
    EXPECT_EQ(found->suffix, " Hz");
    EXPECT_NEAR(found->minimum, 1.0, 1e-9);
    EXPECT_NEAR(found->maximum, 22050.0, 1e-9);
    EXPECT_NEAR(found->step, 10.0, 1e-9);
    EXPECT_EQ(found->decimals, 1u);
}

// The panels this replaces connected their enable handlers after setting the initial selection, so
// the starting state was never synchronised. Emitting it at build time is what fixes that, and
// asserting it here is what stops it regressing.
TEST_F(RecordingFormViewTest, EveryFieldStatesItsInitialVisibilityAndEnablement)
{
    EXPECT_EQ(view.CountOf(FormCommandKind::SetVisible), 5u);
    EXPECT_EQ(view.CountOf(FormCommandKind::SetEnabled), 5u);
    EXPECT_FALSE(view.IsControlEnabled(cutoffHigh));
    EXPECT_TRUE(view.IsControlEnabled(cutoff));
}

TEST_F(RecordingFormViewTest, FlippingAChoiceEnablesOnlyTheFieldThatDependsOnIt)
{
    view.Clear();
    view.PickOption(filterType, 2);

    EXPECT_EQ(view.CountOf(FormCommandKind::SetEnabled), 1u);
    const auto* last = LastOf(FormCommandKind::SetEnabled);
    ASSERT_NE(last, nullptr);
    EXPECT_EQ(last->field, cutoffHigh);
    EXPECT_TRUE(last->flag);
}

TEST_F(RecordingFormViewTest, TypingAValueReachesTheModelAndReportsOneChange)
{
    view.TypeNumber(cutoff, 2500.0);

    EXPECT_NEAR(model.Float(cutoff), 2500.0f, 1e-4f);
    EXPECT_EQ(view.FieldChangeCount(), 1u);
}

// A programmatic write has to move the control without coming back as a user edit, which is what
// makes a write from elsewhere in the application safe.
TEST_F(RecordingFormViewTest, RefreshPushesTheModelOutwardWithoutReportingAnEdit)
{
    model.SetNumber(cutoff, 777.0);

    view.Clear();
    ASSERT_EQ(view.FieldChangeCount(), 0u);

    view.Refresh();

    EXPECT_EQ(view.CountOf(FormCommandKind::SetValue), 3u);
    EXPECT_EQ(view.FieldChangeCount(), 0u);
}

TEST_F(RecordingFormViewTest, RefreshingOneFieldEmitsOnlyThatField)
{
    view.Clear();
    view.Refresh(cutoff);

    ASSERT_EQ(view.Commands().size(), 1u);
    EXPECT_EQ(view.Commands().front().field, cutoff);
}

TEST_F(RecordingFormViewTest, PressingAnActionReachesItsSubscriber)
{
    ActionId triggered{ ui::model::noAction };
    model.onActionTriggered = [&triggered](ActionId id)
    {
        triggered = id;
    };

    view.PressAction(compute);

    EXPECT_EQ(triggered, compute);
}

TEST_F(RecordingFormViewTest, AddingRowsStopsAtTheDeclaredMaximum)
{
    for (auto i = 0; i < 4; ++i)
        EXPECT_TRUE(view.PressAddRow(0));

    EXPECT_FALSE(view.PressAddRow(0));
    EXPECT_EQ(model.Table(0).RowCount(), 4u);
}

TEST_F(RecordingFormViewTest, RemovingARowNotifiesAndCompacts)
{
    view.PressAddRow(0);
    view.PressAddRow(0);
    model.Table(0).SetCell(1, 0, 4242.0);

    auto notified = 0;
    model.onTableChanged = [&notified]
    {
        ++notified;
    };

    ASSERT_TRUE(view.PressRemoveRow(0, 0));

    EXPECT_EQ(notified, 1);
    EXPECT_EQ(model.Table(0).RowCount(), 1u);
    EXPECT_NEAR(model.Table(0).Cell(0, 0), 4242.0, 1e-9);
}

TEST_F(RecordingFormViewTest, AnUnknownFieldIsNeitherVisibleNorEnabled)
{
    EXPECT_FALSE(view.IsControlVisible(FieldId{ 999 }));
    EXPECT_FALSE(view.IsControlEnabled(FieldId{ 999 }));
}

TEST_F(RecordingFormViewTest, TogglingAFlagReachesTheModel)
{
    view.ToggleFlag(normalise, true);

    EXPECT_TRUE(model.Flag(normalise));
    EXPECT_EQ(view.FieldChangeCount(), 1u);
}

TEST_F(RecordingFormViewTest, RefreshingAToggleRecordsItsFlagRatherThanAValue)
{
    model.SetFlag(normalise, true);

    view.Clear();
    view.Refresh(normalise);

    ASSERT_EQ(view.Commands().size(), 1u);
    EXPECT_EQ(view.Commands().front().kind, FormCommandKind::SetFlag);
    EXPECT_TRUE(view.Commands().front().flag);
}

TEST_F(RecordingFormViewTest, RefreshingAChoiceRecordsItsSelection)
{
    model.SetSelection(filterType, 2);

    view.Clear();
    view.Refresh(filterType);

    ASSERT_EQ(view.Commands().size(), 1u);
    EXPECT_EQ(view.Commands().front().kind, FormCommandKind::SetSelection);
    EXPECT_EQ(view.Commands().front().index, 2u);
}

TEST_F(RecordingFormViewTest, DisablingAnActionIsRecorded)
{
    view.Clear();
    view.SetActionEnabled(compute, false);

    ASSERT_EQ(view.Commands().size(), 1u);
    EXPECT_EQ(view.Commands().front().action, compute);
    EXPECT_FALSE(view.Commands().front().flag);
}

TEST_F(RecordingFormViewTest, TheLabelsReportEveryNamedControl)
{
    EXPECT_THAT(view.Labels(), ::testing::IsSupersetOf({ "Cutoff", "Order", "Type", "Cutoff High", "Normalise", "Compute" }));
}

TEST_F(RecordingFormViewTest, DrivingAViewThatWasNeverBuiltIsHarmless)
{
    RecordingFormView unbuilt;

    unbuilt.TypeNumber(cutoff, 1.0);
    unbuilt.Refresh();
    unbuilt.Refresh(cutoff);

    EXPECT_FALSE(unbuilt.PressAddRow(0));
    EXPECT_FALSE(unbuilt.PressRemoveRow(0, 0));
    EXPECT_TRUE(unbuilt.Commands().empty());
}
