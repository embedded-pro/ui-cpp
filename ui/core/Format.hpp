#pragma once

#include <array>
#include <cstdio>
#include <string>
#include <string_view>

namespace ui
{
    // std::format is unavailable on the GCC 11 shipped by the Ubuntu CI image, and Tier 1 carries
    // no external dependency, so fmt is not an option either.
    template<std::size_t Capacity = 64>
    class FormatBuffer
    {
    public:
        [[nodiscard]] std::string_view Fixed(float value, int decimals)
        {
            const auto written = std::snprintf(storage.data(), storage.size(), "%.*f", decimals, static_cast<double>(value));
            return Taken(written);
        }

        [[nodiscard]] std::string_view Scientific(float value, int decimals)
        {
            const auto written = std::snprintf(storage.data(), storage.size(), "%.*e", decimals, static_cast<double>(value));
            return Taken(written);
        }

        [[nodiscard]] std::string_view Integer(long long value)
        {
            const auto written = std::snprintf(storage.data(), storage.size(), "%lld", value);
            return Taken(written);
        }

    private:
        [[nodiscard]] std::string_view Taken(int written)
        {
            if (written < 0)
                return {};

            const auto length = static_cast<std::size_t>(written);
            return std::string_view{ storage.data(), length < storage.size() ? length : storage.size() - 1 };
        }

        std::array<char, Capacity> storage{};
    };

    [[nodiscard]] std::string FormatFixed(float value, int decimals);
    [[nodiscard]] std::string FormatEngineering(float value, int decimals);
}
