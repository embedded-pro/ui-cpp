#pragma once

#include "ui/core/Canvas.hpp"
#include "ui/scene/Camera3D.hpp"

namespace ui::scene
{
    struct GroundGridConfig
    {
        float extent{ 2.0f };
        float step{ 0.25f };
        float lineWidth{ 1.0f };
    };

    struct AxisTriadConfig
    {
        float length{ 0.4f };
        float labelOffset{ 0.05f };
        float lineWidth{ 2.0f };
    };

    void DrawGroundGrid(Canvas& canvas, const ViewFrame& frame, const GroundGridConfig& config = {});
    void DrawAxisTriad(Canvas& canvas, const ViewFrame& frame, const AxisTriadConfig& config = {});
}
