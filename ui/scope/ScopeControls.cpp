#include "ui/scope/ScopeControls.hpp"
#include <algorithm>

namespace ui::scope
{
    namespace
    {
        struct TimePerDivisionOption
        {
            std::string_view label;
            float seconds;
        };

        constexpr std::array<TimePerDivisionOption, timePerDivisionOptions> timeTable{
            TimePerDivisionOption{ "10 µs/div", 10e-6f },
            TimePerDivisionOption{ "20 µs/div", 20e-6f },
            TimePerDivisionOption{ "50 µs/div", 50e-6f },
            TimePerDivisionOption{ "100 µs/div", 100e-6f },
            TimePerDivisionOption{ "200 µs/div", 200e-6f },
            TimePerDivisionOption{ "500 µs/div", 500e-6f },
            TimePerDivisionOption{ "1 ms/div", 1e-3f },
            TimePerDivisionOption{ "2 ms/div", 2e-3f },
            TimePerDivisionOption{ "5 ms/div", 5e-3f },
            TimePerDivisionOption{ "10 ms/div", 10e-3f },
            TimePerDivisionOption{ "20 ms/div", 20e-3f },
            TimePerDivisionOption{ "50 ms/div", 50e-3f },
            TimePerDivisionOption{ "100 ms/div", 100e-3f },
            TimePerDivisionOption{ "200 ms/div", 200e-3f },
            TimePerDivisionOption{ "500 ms/div", 500e-3f },
            TimePerDivisionOption{ "1 s/div", 1.0f }
        };

        constexpr std::array<model::OptionSpec, timePerDivisionOptions> timeOptions{
            model::OptionSpec{ timeTable[0].label, 0 }, model::OptionSpec{ timeTable[1].label, 1 },
            model::OptionSpec{ timeTable[2].label, 2 }, model::OptionSpec{ timeTable[3].label, 3 },
            model::OptionSpec{ timeTable[4].label, 4 }, model::OptionSpec{ timeTable[5].label, 5 },
            model::OptionSpec{ timeTable[6].label, 6 }, model::OptionSpec{ timeTable[7].label, 7 },
            model::OptionSpec{ timeTable[8].label, 8 }, model::OptionSpec{ timeTable[9].label, 9 },
            model::OptionSpec{ timeTable[10].label, 10 }, model::OptionSpec{ timeTable[11].label, 11 },
            model::OptionSpec{ timeTable[12].label, 12 }, model::OptionSpec{ timeTable[13].label, 13 },
            model::OptionSpec{ timeTable[14].label, 14 }, model::OptionSpec{ timeTable[15].label, 15 }
        };

        constexpr std::array<model::OptionSpec, 3> modeOptions{
            model::OptionSpec{ "Auto", static_cast<std::int64_t>(TriggerMode::Auto) },
            model::OptionSpec{ "Normal", static_cast<std::int64_t>(TriggerMode::Normal) },
            model::OptionSpec{ "Single", static_cast<std::int64_t>(TriggerMode::Single) }
        };

        constexpr std::array<model::OptionSpec, 2> edgeOptions{
            model::OptionSpec{ "↑ Rising", static_cast<std::int64_t>(TriggerEdge::Rising) },
            model::OptionSpec{ "↓ Falling", static_cast<std::int64_t>(TriggerEdge::Falling) }
        };

        constexpr std::array<std::string_view, ScopeCore::maxChannels> channelLabels{ "1", "2", "3", "4" };
    }

    float SecondsPerDivisionAt(std::size_t index)
    {
        return timeTable[std::min(index, timeTable.size() - 1)].seconds;
    }

    std::size_t ScopeControls::ChannelsFor(std::size_t requested)
    {
        return std::clamp(requested, std::size_t{ 1 }, ScopeCore::maxChannels);
    }

    std::array<model::OptionSpec, ScopeCore::maxChannels> ScopeControls::MakeChannelOptions(std::size_t channels)
    {
        std::array<model::OptionSpec, ScopeCore::maxChannels> options{};

        for (std::size_t i = 0; i != channels; ++i)
            options[i] = model::OptionSpec{ channelLabels[i], static_cast<std::int64_t>(i) };

        return options;
    }

    std::array<model::FieldSpec, 5> ScopeControls::MakeFields(std::span<const model::OptionSpec> channels)
    {
        using enum model::FieldKind;

        return {
            model::FieldSpec{ field::timePerDivision, model::noGroup, Choice, "Time/div:", "", {}, timeOptions, {}, {} },
            model::FieldSpec{ field::triggerMode, model::noGroup, Choice, "Trigger:", "", {}, modeOptions, {}, {} },
            model::FieldSpec{ field::triggerEdge, model::noGroup, Choice, "Edge:", "", {}, edgeOptions, {}, {} },
            model::FieldSpec{ field::triggerLevel, model::noGroup, Number, "Level:", " A", { -100.0, 100.0, 0.01, 0.0, 3, 0.0 }, {}, {}, {} },
            model::FieldSpec{ field::triggerChannel, model::noGroup, Choice, "Ch:", "", {}, channels, {}, {} }
        };
    }

    std::array<model::ActionSpec, 3> ScopeControls::MakeActions()
    {
        using enum theme::ButtonRole;

        return {
            model::ActionSpec{ field::runStop, stopLabel, Stop, 0 },
            model::ActionSpec{ field::single, "Single", Primary, 0 },
            model::ActionSpec{ field::force, "Force", Default, 0 }
        };
    }

    ScopeControls::ScopeControls(std::size_t channelCount)
        : ScopeControls(ChannelCount{ ChannelsFor(channelCount) })
    {}

    ScopeControls::ScopeControls(ChannelCount channels)
        : channelOptions(MakeChannelOptions(channels.value))
        , fields(MakeFields(std::span{ channelOptions }.first(channels.value)))
        , spec(model::FormSpec{ {}, fields, actions, {} })
        , model(spec, values, {})
    {
        model.SetSelection(field::timePerDivision, defaultTimePerDivision);
    }

    model::FormModel& ScopeControls::Model()
    {
        return model;
    }

    const model::FormSpec& ScopeControls::Spec() const
    {
        return spec;
    }
}
