#pragma once

#include "ui/model/FormModel.hpp"
#include "ui/scope/ScopeCore.hpp"
#include <array>
#include <cstddef>
#include <span>
#include <string_view>

namespace ui::scope
{
    namespace field
    {
        inline constexpr model::FieldId timePerDivision{ 1 };
        inline constexpr model::FieldId triggerMode{ 2 };
        inline constexpr model::FieldId triggerEdge{ 3 };
        inline constexpr model::FieldId triggerLevel{ 4 };
        inline constexpr model::FieldId triggerChannel{ 5 };

        inline constexpr model::ActionId runStop{ 1 };
        inline constexpr model::ActionId single{ 2 };
        inline constexpr model::ActionId force{ 3 };
    }

    inline constexpr std::size_t timePerDivisionOptions{ 16 };
    inline constexpr std::size_t defaultTimePerDivision{ 9 };

    inline constexpr std::string_view runLabel{ "Run" };
    inline constexpr std::string_view stopLabel{ "Stop" };

    [[nodiscard]] float SecondsPerDivisionAt(std::size_t index);

    class ScopeControls
    {
    public:
        explicit ScopeControls(std::size_t channelCount);

        [[nodiscard]] model::FormModel& Model();
        [[nodiscard]] const model::FormSpec& Spec() const;

    private:
        struct ChannelCount
        {
            std::size_t value{ 1 };
        };

        explicit ScopeControls(ChannelCount channels);

        [[nodiscard]] static std::size_t ChannelsFor(std::size_t requested);
        [[nodiscard]] static std::array<model::OptionSpec, ScopeCore::maxChannels> MakeChannelOptions(std::size_t channels);
        [[nodiscard]] static std::array<model::FieldSpec, 5> MakeFields(std::span<const model::OptionSpec> channels);
        [[nodiscard]] static std::array<model::ActionSpec, 3> MakeActions();

        std::array<model::OptionSpec, ScopeCore::maxChannels> channelOptions;
        std::array<model::FieldSpec, 5> fields;
        std::array<model::ActionSpec, 3> actions{ MakeActions() };
        std::array<model::FieldValue, 5> values{};

        model::FormSpec spec;
        model::FormModel model;
    };
}
