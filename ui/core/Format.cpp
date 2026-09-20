#include "ui/core/Format.hpp"
#include "ui/core/Geometry.hpp"
#include <cmath>

namespace ui
{
    std::string FormatFixed(float value, int decimals)
    {
        FormatBuffer<64> buffer;
        return std::string{ buffer.Fixed(value, decimals) };
    }

    std::string FormatEngineering(float value, int decimals)
    {
        static constexpr std::string_view prefixes[]{ "p", "n", "u", "m", "", "k", "M", "G" };
        static constexpr int unityIndex{ 4 };

        if (value == 0.0f || !std::isfinite(value))
            return FormatFixed(value, decimals);

        auto exponent = static_cast<int>(std::floor(std::log10(std::fabs(value)) / 3.0f));
        exponent = static_cast<int>(Clamp(static_cast<float>(exponent), -unityIndex, 3.0f));

        const auto scaled = value / std::pow(10.0f, static_cast<float>(exponent * 3));

        FormatBuffer<64> buffer;
        return std::string{ buffer.Fixed(scaled, decimals) } + std::string{ prefixes[exponent + unityIndex] };
    }
}
