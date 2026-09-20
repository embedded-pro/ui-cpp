#include "ui/charts/ChartInteraction.hpp"
#include <algorithm>

namespace ui::charts
{
    void ChartInteraction::SetDataRange(float minimum, float maximum)
    {
        dataMinimum = minimum;
        dataMaximum = maximum;
        viewMinimum = minimum;
        viewMaximum = maximum;
    }

    void ChartInteraction::ResetView()
    {
        viewMinimum = dataMinimum;
        viewMaximum = dataMaximum;
    }

    void ChartInteraction::Zoom(float wheelDelta, float cursorRatio)
    {
        const auto span = viewMaximum - viewMinimum;
        if (span <= 0.0f)
            return;

        const auto factor = wheelDelta > 0.0f ? 1.0f / zoomFactor : zoomFactor;
        const auto newSpan = std::clamp(span * factor, minimumViewSpan, dataMaximum - dataMinimum);

        const auto anchor = viewMinimum + cursorRatio * span;
        viewMinimum = anchor - cursorRatio * newSpan;
        viewMaximum = anchor + (1.0f - cursorRatio) * newSpan;

        ClampView();
    }

    void ChartInteraction::StartPan(Point position)
    {
        panning = true;
        panStartPosition = position;
        panStartViewMinimum = viewMinimum;
        panStartViewMaximum = viewMaximum;
    }

    void ChartInteraction::UpdatePan(Point position, float plotWidth)
    {
        if (!panning || plotWidth <= 0.0f)
            return;

        const auto span = panStartViewMaximum - panStartViewMinimum;
        const auto pixelDelta = position.x - panStartPosition.x;
        const auto viewDelta = -pixelDelta / plotWidth * span;

        viewMinimum = panStartViewMinimum + viewDelta;
        viewMaximum = panStartViewMaximum + viewDelta;

        ClampView();
    }

    void ChartInteraction::EndPan()
    {
        panning = false;
    }

    float ChartInteraction::ViewMinimum() const
    {
        return viewMinimum;
    }

    float ChartInteraction::ViewMaximum() const
    {
        return viewMaximum;
    }

    float ChartInteraction::ViewSpan() const
    {
        return viewMaximum - viewMinimum;
    }

    void ChartInteraction::ShowCrosshairAt(Point position)
    {
        showCrosshair = true;
        cursorPosition = position;
    }

    void ChartInteraction::HideCrosshair()
    {
        showCrosshair = false;
    }

    bool ChartInteraction::CrosshairVisible() const
    {
        return showCrosshair;
    }

    Point ChartInteraction::CursorPosition() const
    {
        return cursorPosition;
    }

    bool ChartInteraction::IsZoomed() const
    {
        const auto tolerance = (dataMaximum - dataMinimum) * 0.001f;
        return (viewMinimum - dataMinimum) > tolerance || (dataMaximum - viewMaximum) > tolerance;
    }

    bool ChartInteraction::IsPanning() const
    {
        return panning;
    }

    void ChartInteraction::ClampView()
    {
        const auto span = viewMaximum - viewMinimum;
        const auto maximumSpan = dataMaximum - dataMinimum;

        if (span > maximumSpan)
        {
            viewMinimum = dataMinimum;
            viewMaximum = dataMaximum;
            return;
        }

        if (viewMinimum < dataMinimum)
        {
            viewMinimum = dataMinimum;
            viewMaximum = viewMinimum + span;
        }

        if (viewMaximum > dataMaximum)
        {
            viewMaximum = dataMaximum;
            viewMinimum = viewMaximum - span;
        }
    }
}
