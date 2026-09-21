#include "ui/model/FormModel.hpp"
#include <array>
#include <gmock/gmock.h>

namespace
{
    using ui::model::Condition;
    using ui::model::FieldId;
    using ui::model::FieldKind;
    using ui::model::FieldSpec;
    using ui::model::FieldValue;
    using ui::model::FormModel;
    using ui::model::FormSpec;
    using ui::model::OptionSpec;
    using ui::model::Violation;
    using ui::model::ViolationKind;

    constexpr FieldId gain{ 1 };
    constexpr FieldId mode{ 2 };
    constexpr FieldId conditional{ 3 };

    constexpr std::array<OptionSpec, 2> modeOptions{
        OptionSpec{ "Off" },
        OptionSpec{ "On" }
    };

    constexpr std::array<FieldSpec, 3> fields{
        FieldSpec{ gain, ui::model::noGroup, FieldKind::Number, "Gain", "", { 0.0, 10.0, 0.1, 1.0, 2 }, {}, {}, {} },
        FieldSpec{ mode, ui::model::noGroup, FieldKind::Choice, "Mode", "", {}, modeOptions, {}, {} },
        FieldSpec{ conditional, ui::model::noGroup, FieldKind::Number, "Only When On", "", { 0.0, 1.0, 0.1, 0.0, 2 }, {}, Condition{ mode, 0b10u }, {} }
    };

    class ValidationTest
        : public ::testing::Test
    {
    protected:
        std::array<FieldValue, 3> storage{};
        FormSpec spec{ {}, fields, {}, {} };
        FormModel model{ spec, storage, {} };
    };
}

TEST_F(ValidationTest, AFreshFormIsValid)
{
    EXPECT_EQ(model.Validate(), std::nullopt);
}

// A backend that clamps its controls never produces these; they exist for values written
// programmatically, and for a backend that does not clamp.
TEST_F(ValidationTest, AValueBelowTheMinimumIsReported)
{
    model.SetNumber(gain, -1.0);

    EXPECT_EQ(model.Validate(), (Violation{ gain, ViolationKind::BelowMinimum }));
}

TEST_F(ValidationTest, AValueAboveTheMaximumIsReported)
{
    model.SetNumber(gain, 11.0);

    EXPECT_EQ(model.Validate(), (Violation{ gain, ViolationKind::AboveMaximum }));
}

TEST_F(ValidationTest, AChoiceWithNoOptionsIsReported)
{
    constexpr std::array<FieldSpec, 1> emptyChoice{
        FieldSpec{ mode, ui::model::noGroup, FieldKind::Choice, "Mode", "", {}, {}, {}, {} }
    };

    std::array<FieldValue, 1> localStorage{};
    const FormSpec localSpec{ {}, emptyChoice, {}, {} };
    const FormModel local{ localSpec, localStorage, {} };

    EXPECT_EQ(local.Validate(), (Violation{ mode, ViolationKind::NoSelection }));
}

// A hidden field is not the user's problem, so it is not validated - which is also what keeps the
// swapped plant groups from reporting violations for the branch that is not showing.
TEST_F(ValidationTest, AHiddenFieldIsNotValidated)
{
    model.SetNumber(conditional, 99.0);

    ASSERT_FALSE(model.IsVisible(conditional));
    EXPECT_EQ(model.Validate(), std::nullopt);

    model.SetSelection(mode, 1);

    EXPECT_EQ(model.Validate(), (Violation{ conditional, ViolationKind::AboveMaximum }));
}
