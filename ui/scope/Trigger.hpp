#pragma once

#include "ui/core/Color.hpp"
#include <cstdint>
#include <string>

namespace ui::scope
{
    enum class TriggerMode : std::uint8_t
    {
        Auto,
        Normal,
        Single
    };

    enum class TriggerEdge : std::uint8_t
    {
        Rising,
        Falling
    };

    struct ChannelConfig
    {
        std::string name;
        Color color;
        bool enabled{ true };
    };

    // The divisions are the instrument's face, not a layout detail: a 10x8 graticule is what makes
    // a reading like "5 ms/div" mean anything, so it is configuration rather than a theme metric.
    struct ScopeConfig
    {
        std::size_t horizontalDivisions{ 10 };
        std::size_t verticalDivisions{ 8 };
        std::size_t sampleCapacity{ 256 * 1024 };

        float leftMargin{ 60.0f };
        float rightMargin{ 20.0f };
        float topMargin{ 10.0f };
        float bottomMargin{ 30.0f };

        // Fraction of the sweep shown before the trigger point, so the edge that fired it is
        // visible rather than sitting on the left border.
        float preTriggerFraction{ 0.2f };

        // How far back a re-arm searches for the most recent qualifying edge.
        std::size_t triggerSearchLimit{ 65536 };
    };
}
