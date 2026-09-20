#pragma once

#include "ui/core/Geometry.hpp"

namespace ui::charts
{
    // Operates entirely in view space, which is linear for every axis transform, so this is shared
    // unchanged between the time-domain and frequency-domain charts.
    class ChartInteraction
    {
    public:
        void SetDataRange(float minimum, float maximum);
        void ResetView();
        void Zoom(float wheelDelta, float cursorRatio);
        void StartPan(Point position);
        void UpdatePan(Point position, float plotWidth);
        void EndPan();

        [[nodiscard]] bool IsZoomed() const;
        [[nodiscard]] bool IsPanning() const;

        float viewMinimum{ 0.0f };
        float viewMaximum{ 1.0f };

        bool showCrosshair{ false };
        Point cursorPosition;

    private:
        void ClampView();

        float dataMinimum{ 0.0f };
        float dataMaximum{ 1.0f };
        bool panning{ false };
        Point panStartPosition;
        float panStartViewMinimum{ 0.0f };
        float panStartViewMaximum{ 0.0f };

        static constexpr float zoomFactor{ 1.15f };
        static constexpr float minimumViewSpan{ 1e-4f };
    };
}
