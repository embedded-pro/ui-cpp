#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace ui::charts
{
    struct AxisRange
    {
        float minimum{ 0.0f };
        float maximum{ 0.0f };

        [[nodiscard]] constexpr float Span() const
        {
            return maximum - minimum;
        }
    };

    struct Tick
    {
        float viewPosition{ 0.0f };
        std::array<char, 24> text{};
        std::uint8_t length{ 0 };

        [[nodiscard]] std::string_view Label() const
        {
            return std::string_view{ text.data(), length };
        }

        void SetLabel(std::string_view label);
    };

    struct GridLine
    {
        float viewPosition{ 0.0f };
        bool major{ true };
    };

    // The seam between the time-domain and frequency-domain charts. Everything the two
    // implementations differed in is expressed here; the chart engine itself is axis-agnostic and
    // works entirely in view space, where both are linear.
    class AxisTransform
    {
    public:
        AxisTransform() = default;
        AxisTransform(const AxisTransform& other) = delete;
        AxisTransform& operator=(const AxisTransform& other) = delete;
        virtual ~AxisTransform() = default;

        [[nodiscard]] virtual float ToView(float value) const = 0;
        [[nodiscard]] virtual float FromView(float view) const = 0;
        [[nodiscard]] virtual bool IsPlottable(float value) const = 0;

        [[nodiscard]] virtual AxisRange RangeFor(std::span<const float> values) const = 0;

        [[nodiscard]] virtual std::size_t Ticks(AxisRange view, std::span<Tick> out) const = 0;
        [[nodiscard]] virtual std::size_t GridLines(AxisRange view, std::span<GridLine> out) const = 0;

        [[nodiscard]] virtual std::string_view Title() const = 0;
        [[nodiscard]] virtual std::size_t FormatCursorValue(float value, std::span<char> out) const = 0;
    };
}
