#pragma once

#include <array>
#include <format>
#include <iterator>
#include <span>
#include <string>
#include <string_view>

namespace ui
{
    // std::format_to_n takes a signed count, so the cast lives here once rather than at each call.
    template<class... Arguments>
    [[nodiscard]] std::size_t FormatInto(std::span<char> out, std::format_string<Arguments...> pattern, Arguments&&... arguments)
    {
        const auto capacity = static_cast<std::iter_difference_t<char*>>(out.size());
        const auto result = std::format_to_n(out.data(), capacity, pattern, std::forward<Arguments>(arguments)...);

        return std::min(static_cast<std::size_t>(result.size), out.size());
    }

    // std::format_to_n rather than std::format: this is used inside Paint, which must not allocate,
    // so the result is written into a caller-owned buffer.
    template<std::size_t Capacity = 64>
    class FormatBuffer
    {
    public:
        [[nodiscard]] std::string_view Fixed(float value, int decimals)
        {
            return Write("{:.{}f}", value, decimals);
        }

        // Precision here is significant digits rather than places, which is what {:g} means by it.
        [[nodiscard]] std::string_view Significant(float value, int digits)
        {
            return Write("{:.{}g}", value, digits);
        }

        [[nodiscard]] std::string_view Scientific(float value, int decimals)
        {
            return Write("{:.{}e}", value, decimals);
        }

        [[nodiscard]] std::string_view Integer(long long value)
        {
            return Write("{}", value);
        }

    private:
        template<class... Arguments>
        [[nodiscard]] std::string_view Write(std::format_string<Arguments...> pattern, Arguments&&... arguments)
        {
            const auto written = FormatInto(storage, pattern, std::forward<Arguments>(arguments)...);
            return std::string_view{ storage.data(), written };
        }

        std::array<char, Capacity> storage{};
    };

    [[nodiscard]] std::string FormatFixed(float value, int decimals);
    [[nodiscard]] std::string FormatEngineering(float value, int decimals);
}
