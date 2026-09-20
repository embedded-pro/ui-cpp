#include "ui/charts/AxisTransform.hpp"
#include <algorithm>

namespace ui::charts
{
    void Tick::SetLabel(std::string_view label)
    {
        length = static_cast<std::uint8_t>(std::min(label.size(), text.size()));
        std::copy_n(label.begin(), length, text.begin());
    }
}
