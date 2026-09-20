#pragma once

#include "ui/core/Color.hpp"
#include <string>
#include <vector>

namespace ui::charts
{
    struct Series
    {
        std::string name;
        Color color;
        std::vector<float> data;
    };

    struct ChartPanel
    {
        std::string title;
        std::string yAxisLabel;
        std::vector<Series> series;
        int heightWeight{ 1 };
    };

    struct PanelBounds
    {
        float minimumY{ 0.0f };
        float maximumY{ 0.0f };

        [[nodiscard]] constexpr float Span() const
        {
            return maximumY - minimumY;
        }
    };
}
