#pragma once

#include "ui/model/FormModel.hpp"
#include <array>

// One spec shared by the Qt tests and the conformance suite, so both implementations are asked
// about the same form rather than about two that merely resemble each other.
namespace formspec
{
    inline constexpr ui::model::FieldId cutoff{ 1 };
    inline constexpr ui::model::FieldId order{ 2 };
    inline constexpr ui::model::FieldId filterType{ 3 };
    inline constexpr ui::model::FieldId cutoffHigh{ 4 };
    inline constexpr ui::model::FieldId normalise{ 5 };
    inline constexpr ui::model::FieldId gainReadOut{ 6 };
    inline constexpr ui::model::FieldId torque{ 7 };
    inline constexpr ui::model::GroupId plant{ 10 };
    inline constexpr ui::model::ActionId compute{ 1 };

    inline constexpr std::array<ui::model::OptionSpec, 3> filterOptions{
        ui::model::OptionSpec{ "Low-Pass", 10 },
        ui::model::OptionSpec{ "High-Pass", 20 },
        ui::model::OptionSpec{ "Band-Pass", 30 }
    };

    inline constexpr std::array<ui::model::GroupSpec, 1> groups{
        ui::model::GroupSpec{ plant, "Plant", ui::model::Condition{ filterType, 0b100u } }
    };

    // The label carries a multi-byte glyph deliberately: it is how the UTF-8 path from a literal
    // through to a Qt control stays covered on every platform the tests run on.
    inline constexpr std::array<ui::model::FieldSpec, 7> fields{
        ui::model::FieldSpec{ cutoff, ui::model::noGroup, ui::model::FieldKind::Number, "Cutoff (λ)", " Hz", { 1.0, 22050.0, 10.0, 1000.0, 1 }, {}, {}, {} },
        ui::model::FieldSpec{ order, ui::model::noGroup, ui::model::FieldKind::Integer, "Order", "", { 3.0, 127.0, 2.0, 31.0, 0 }, {}, {}, {} },
        ui::model::FieldSpec{ filterType, ui::model::noGroup, ui::model::FieldKind::Choice, "Type", "", {}, filterOptions, {}, {} },
        ui::model::FieldSpec{ cutoffHigh, ui::model::noGroup, ui::model::FieldKind::Number, "Cutoff High", " Hz", { 1.0, 22050.0, 10.0, 4000.0, 1 }, {}, {}, ui::model::Condition{ filterType, 0b100u } },
        ui::model::FieldSpec{ normalise, ui::model::noGroup, ui::model::FieldKind::Toggle, "Normalise", "", {}, {}, {}, {} },
        ui::model::FieldSpec{ gainReadOut, plant, ui::model::FieldKind::ReadOut, "Gain", " dB", { 0.0, 100.0, 0.0, 0.0, 2 }, {}, {}, {} },
        ui::model::FieldSpec{ torque, ui::model::noGroup, ui::model::FieldKind::Slider, "Torque", "", { -200.0, 200.0, 1.0, 0.0, 0, 50.0 }, {}, {}, {} }
    };

    inline constexpr std::array<ui::model::ActionSpec, 1> actions{
        ui::model::ActionSpec{ compute, "Compute", ui::theme::ButtonRole::Primary, 40 }
    };

    inline constexpr std::array<ui::model::ColumnSpec, 2> columns{
        ui::model::ColumnSpec{ "Frequency (Hz)", { 0.0, 96000.0, 10.0, 1000.0, 1 } },
        ui::model::ColumnSpec{ "Amplitude", { 0.0, 1.0, 0.01, 0.5, 3 } }
    };

    // Storage for a whole form, so a fixture is three declarations rather than eight.
    class Harness
    {
    public:
        explicit Harness(ui::model::FormLayout layout = ui::model::FormLayout::Stacked)
            : spec{ groups, fields, actions, tableSpecs, layout }
            , model{ spec, values, tables }
        {
            model.Table(0).AddRow();
        }

        [[nodiscard]] ui::model::FormModel& Model()
        {
            return model;
        }

    private:
        std::array<ui::model::TableSpec, 1> tableSpecs{
            ui::model::TableSpec{ ui::model::noGroup, columns, 4, "Add", "Remove" }
        };

        std::array<double, 8> cells{};
        std::array<ui::model::TableModel, 1> tables{ ui::model::TableModel{ tableSpecs[0], cells } };
        std::array<ui::model::FieldValue, 7> values{};

        ui::model::FormSpec spec;
        ui::model::FormModel model;
    };
}
