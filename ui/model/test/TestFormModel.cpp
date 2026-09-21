#include "ui/model/FormModel.hpp"
#include <array>
#include <gmock/gmock.h>

namespace
{
    using ui::model::ActionId;
    using ui::model::Condition;
    using ui::model::FieldId;
    using ui::model::FieldKind;
    using ui::model::FieldSpec;
    using ui::model::FieldValue;
    using ui::model::FormModel;
    using ui::model::FormSpec;
    using ui::model::GroupId;
    using ui::model::GroupSpec;
    using ui::model::OptionSpec;
    using ui::model::TableModel;

    constexpr FieldId cutoff{ 1 };
    constexpr FieldId order{ 2 };
    constexpr FieldId filterType{ 3 };
    constexpr FieldId cutoffHigh{ 4 };
    constexpr FieldId normalise{ 5 };
    constexpr FieldId readout{ 6 };
    constexpr GroupId plant{ 10 };

    constexpr std::array<OptionSpec, 3> filterOptions{
        OptionSpec{ "Low-Pass", 10 },
        OptionSpec{ "High-Pass", 20 },
        OptionSpec{ "Band-Pass", 30 }
    };

    // Options with no data carry their own index, which is how the panels that switched on
    // currentIndex() and the ones that read currentData() end up with one read path.
    constexpr std::array<OptionSpec, 2> plantOptions{
        OptionSpec{ "First Order" },
        OptionSpec{ "Second Order" }
    };

    constexpr std::array<GroupSpec, 1> groups{
        GroupSpec{ plant, "Plant", Condition{ filterType, 0b100u } }
    };

    constexpr std::array<FieldSpec, 6> fields{
        FieldSpec{ cutoff, ui::model::noGroup, FieldKind::Number, "Cutoff", " Hz", { 1.0, 22050.0, 10.0, 1000.0, 1 }, {}, {}, {} },
        FieldSpec{ order, ui::model::noGroup, FieldKind::Integer, "Order", "", { 3.0, 127.0, 2.0, 31.0, 0 }, {}, {}, {} },
        FieldSpec{ filterType, ui::model::noGroup, FieldKind::Choice, "Type", "", {}, filterOptions, {}, {} },
        FieldSpec{ cutoffHigh, ui::model::noGroup, FieldKind::Number, "Cutoff High", " Hz", { 1.0, 22050.0, 10.0, 4000.0, 1 }, {}, {}, Condition{ filterType, 0b100u } },
        FieldSpec{ normalise, ui::model::noGroup, FieldKind::Toggle, "Normalise", "", {}, {}, {}, {} },
        FieldSpec{ readout, plant, FieldKind::ReadOut, "Gain", " dB", { 0.0, 0.0, 0.0, 0.0, 2 }, {}, {}, {} }
    };

    class FormModelTest
        : public ::testing::Test
    {
    protected:
        std::array<FieldValue, 6> storage{};
        FormSpec spec{ groups, fields, {}, {} };
        FormModel model{ spec, storage, {} };
    };

    class ChoiceDefaultTest
        : public ::testing::Test
    {
    protected:
        std::array<FieldSpec, 1> plantFields{
            FieldSpec{ filterType, ui::model::noGroup, FieldKind::Choice, "Plant", "", {}, plantOptions, {}, {} }
        };

        std::array<FieldValue, 1> storage{};
        FormSpec spec{ {}, plantFields, {}, {} };
        FormModel model{ spec, storage, {} };
    };
}

TEST_F(FormModelTest, ConstructionSeedsEveryFieldFromItsSpec)
{
    EXPECT_NEAR(model.Number(cutoff), 1000.0, 1e-9);
    EXPECT_EQ(model.Count(order), 31u);
    EXPECT_EQ(model.Selection(filterType), 0u);
    EXPECT_FALSE(model.Flag(normalise));
}

TEST_F(FormModelTest, FloatIsTheNarrowingAccessorTheConsumersUse)
{
    model.SetNumber(cutoff, 2500.5);

    EXPECT_NEAR(model.Float(cutoff), 2500.5f, 1e-4f);
}

// The panels expressed an integer two ways - a genuine spinner, and a double spinner with zero
// decimals cast on read. Count() has to serve both.
TEST_F(FormModelTest, CountRoundsRatherThanTruncates)
{
    model.SetNumber(order, 30.6);

    EXPECT_EQ(model.Count(order), 31u);
}

TEST_F(FormModelTest, ANegativeCountClampsToZeroRatherThanWrapping)
{
    model.SetNumber(order, -4.0);

    EXPECT_EQ(model.Count(order), 0u);
}

TEST_F(FormModelTest, SelectedDataReturnsTheOptionsOwnData)
{
    model.SetSelection(filterType, 2);

    EXPECT_EQ(model.SelectedData(filterType), 30);
}

TEST_F(ChoiceDefaultTest, AnOptionWithoutDataReportsItsIndex)
{
    model.SetSelection(filterType, 1);

    EXPECT_EQ(model.SelectedData(filterType), 1);
}

TEST_F(FormModelTest, SelectByDataFindsTheMatchingOption)
{
    EXPECT_TRUE(model.SelectByData(filterType, 20));
    EXPECT_EQ(model.Selection(filterType), 1u);
}

TEST_F(FormModelTest, SelectByDataLeavesTheSelectionAloneWhenNothingMatches)
{
    model.SetSelection(filterType, 1);

    EXPECT_FALSE(model.SelectByData(filterType, 999));
    EXPECT_EQ(model.Selection(filterType), 1u);
}

TEST_F(FormModelTest, AnOutOfRangeSelectionIsRefused)
{
    model.SetSelection(filterType, 7);

    EXPECT_EQ(model.Selection(filterType), 0u);
}

TEST_F(FormModelTest, EveryWriteNotifiesExactlyOnce)
{
    std::vector<FieldId> changed;
    model.onFieldChanged = [&changed](FieldId id)
    {
        changed.push_back(id);
    };

    model.SetNumber(cutoff, 50.0);
    model.SetSelection(filterType, 1);
    model.SetFlag(normalise, true);

    ASSERT_EQ(changed.size(), 3u);
    EXPECT_EQ(changed[0], cutoff);
    EXPECT_EQ(changed[1], filterType);
    EXPECT_EQ(changed[2], normalise);
}

TEST_F(FormModelTest, AnUnconditionalFieldIsAlwaysVisibleAndEnabled)
{
    EXPECT_TRUE(model.IsVisible(cutoff));
    EXPECT_TRUE(model.IsEnabled(cutoff));
}

// The FIR panel enabled its upper cutoff only for a band-pass; the same Condition drives both
// enablement and visibility, so proving one proves the machinery for the other.
TEST_F(FormModelTest, AConditionalFieldTracksTheChoiceItDependsOn)
{
    EXPECT_FALSE(model.IsEnabled(cutoffHigh));

    model.SetSelection(filterType, 2);
    EXPECT_TRUE(model.IsEnabled(cutoffHigh));

    model.SetSelection(filterType, 1);
    EXPECT_FALSE(model.IsEnabled(cutoffHigh));
}

// A negated condition is the complement of the mask, which is why no second operator exists.
TEST_F(FormModelTest, ANegatedConditionIsExpressedAsAMultiBitMask)
{
    const Condition notLowPass{ filterType, 0b110u };
    const std::array<FieldSpec, 2> localFields{
        fields[2],
        FieldSpec{ cutoffHigh, ui::model::noGroup, FieldKind::Number, "Amplitude", "", {}, {}, {}, notLowPass }
    };

    std::array<FieldValue, 2> localStorage{};
    const FormSpec localSpec{ {}, localFields, {}, {} };
    FormModel local{ localSpec, localStorage, {} };

    EXPECT_FALSE(local.IsEnabled(cutoffHigh));

    local.SetSelection(filterType, 1);
    EXPECT_TRUE(local.IsEnabled(cutoffHigh));

    local.SetSelection(filterType, 2);
    EXPECT_TRUE(local.IsEnabled(cutoffHigh));
}

TEST_F(FormModelTest, AFieldInsideAHiddenGroupIsHiddenWithIt)
{
    EXPECT_FALSE(model.IsGroupVisible(plant));
    EXPECT_FALSE(model.IsVisible(readout));

    model.SetSelection(filterType, 2);

    EXPECT_TRUE(model.IsGroupVisible(plant));
    EXPECT_TRUE(model.IsVisible(readout));
}

TEST_F(FormModelTest, ResetToDefaultsRestoresEveryKind)
{
    model.SetNumber(cutoff, 5.0);
    model.SetSelection(filterType, 2);
    model.SetFlag(normalise, true);

    model.ResetToDefaults();

    EXPECT_NEAR(model.Number(cutoff), 1000.0, 1e-9);
    EXPECT_EQ(model.Selection(filterType), 0u);
    EXPECT_FALSE(model.Flag(normalise));
}

TEST_F(FormModelTest, AnActionReachesItsSubscriber)
{
    ActionId triggered{ ui::model::noAction };
    model.onActionTriggered = [&triggered](ActionId id)
    {
        triggered = id;
    };

    model.TriggerAction(ActionId{ 7 });

    EXPECT_EQ(triggered, ActionId{ 7 });
}

TEST_F(FormModelTest, WritingAnUnknownFieldIsIgnoredRatherThanFatal)
{
    auto notified = 0;
    model.onFieldChanged = [&notified](FieldId)
    {
        ++notified;
    };

    model.SetNumber(FieldId{ 999 }, 1.0);

    EXPECT_EQ(notified, 0);
    EXPECT_NEAR(model.Number(FieldId{ 999 }), 0.0, 1e-9);
}
