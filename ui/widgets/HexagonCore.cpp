#include "ui/widgets/HexagonCore.hpp"
#include "ui/core/Format.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <string_view>

namespace ui::widgets
{
    namespace
    {
        constexpr float pi{ std::numbers::pi_v<float> };
        constexpr float piOverThree{ pi / 3.0f };
        constexpr float twoThirds{ 2.0f / 3.0f };
        constexpr float invSqrt3{ std::numbers::inv_sqrt3_v<float> };
        constexpr float twoPiOverThree{ 2.0f * pi / 3.0f };

        constexpr std::array<float, 3> phaseAngles{ 0.0f, twoPiOverThree, -twoPiOverThree };

        constexpr std::size_t corners{ 6 };

        int LabelDecimals(float stepVolts)
        {
            if (stepVolts >= 1.0f)
                return 0;

            if (stepVolts >= 0.1f)
                return 1;

            return 2;
        }

        Point ToScreen(Point centre, float vx, float vy, float scale)
        {
            return Point{ centre.x + vx * scale, centre.y - vy * scale };
        }
    }

    HexagonCore::HexagonCore(const HexagonConfig& config)
        : config(config)
    {}

    void HexagonCore::SetDcLinkVolts(float volts)
    {
        dcLinkVolts = volts;
        RequestRepaint();
    }

    void HexagonCore::SetSample(float va, float vb, float vc, float vAlpha, float vBeta)
    {
        vaSample = va;
        vbSample = vb;
        vcSample = vc;
        vAlphaSample = vAlpha;
        vBetaSample = vBeta;

        const auto magnitude = std::hypot(vAlpha, vBeta);
        const auto rate = magnitude > peakMagnitude ? config.attack : config.decay;
        peakMagnitude += rate * (magnitude - peakMagnitude);
    }

    void HexagonCore::Clear()
    {
        vaSample = 0.0f;
        vbSample = 0.0f;
        vcSample = 0.0f;
        vAlphaSample = 0.0f;
        vBetaSample = 0.0f;
        peakMagnitude = 0.0f;
        RequestRepaint();
    }

    int HexagonCore::SectorOf(float vAlpha, float vBeta)
    {
        if (std::hypot(vAlpha, vBeta) < 1e-6f)
            return 0;

        auto angle = std::atan2(vBeta, vAlpha);

        if (angle < 0.0f)
            angle += 2.0f * pi;

        return static_cast<int>(angle / piOverThree) + 1;
    }

    float HexagonCore::TickStep(float axisHalfRange, std::size_t targetTicks)
    {
        if (axisHalfRange <= 0.0f || targetTicks == 0)
            return 1.0f;

        const auto rawStep = axisHalfRange / static_cast<float>(targetTicks);
        const auto decade = std::pow(10.0f, std::floor(std::log10(rawStep)));
        const auto normalised = rawStep / decade;

        auto mantissa = 1.0f;

        if (normalised >= 5.0f)
            mantissa = 5.0f;
        else if (normalised >= 2.0f)
            mantissa = 2.0f;

        return mantissa * decade;
    }

    Point HexagonCore::VertexOffset(std::size_t corner, float radius)
    {
        const auto angle = static_cast<float>(corner) * piOverThree;
        return Point{ radius * std::cos(angle), -radius * std::sin(angle) };
    }

    float HexagonCore::PeakMagnitude() const
    {
        return peakMagnitude;
    }

    float HexagonCore::DcLinkVolts() const
    {
        return dcLinkVolts;
    }

    float HexagonCore::AxisHalfRange() const
    {
        return dcLinkVolts > 0.0f ? twoThirds * dcLinkVolts : 1.0f;
    }

    float HexagonCore::InscribedRadius() const
    {
        return dcLinkVolts > 0.0f ? invSqrt3 * dcLinkVolts : 1.0f;
    }

    Size HexagonCore::MinimumSize() const
    {
        return Size{ 520.0f, 520.0f };
    }

    void HexagonCore::Paint(Canvas& canvas, const Rect& bounds)
    {
        const auto side = std::min(bounds.width, bounds.height) - 2.0f * config.plotMargin;

        if (side <= 0.0f)
            return;

        const Rect plotArea{ bounds.x + (bounds.width - side) / 2.0f, bounds.y + (bounds.height - side) / 2.0f, side, side };
        const Point centre{ plotArea.x + side / 2.0f, plotArea.y + side / 2.0f };
        const auto scale = side / (2.0f * AxisHalfRange());

        canvas.SetAntialiasing(true);

        DrawAxes(canvas, plotArea, centre, scale);
        DrawHexagon(canvas, centre, scale);
        DrawInscribedCircle(canvas, centre, scale);
        DrawPhasePhasors(canvas, centre, scale);
        DrawResultant(canvas, centre, scale);
        DrawReadOut(canvas, plotArea);
    }

    void HexagonCore::DrawTicks(Canvas& canvas, const Rect& plotArea, Point centre, float scale, float stepVolts) const
    {
        const auto axisHalfRange = (plotArea.width / 2.0f) / scale;
        const auto decimals = LabelDecimals(stepVolts);
        const auto firstTick = static_cast<int>(std::floor(-axisHalfRange / stepVolts)) + 1;
        const auto lastTick = static_cast<int>(std::floor(axisHalfRange / stepVolts));

        FormatBuffer<32> buffer;

        for (auto k = firstTick; k <= lastTick; ++k)
        {
            if (k == 0)
                continue;

            const auto volts = static_cast<float>(k) * stepVolts;
            const auto x = centre.x + volts * scale;
            const auto y = centre.y - volts * scale;

            canvas.SetPen(Pen{ config.palette.grid, 1.0f, LineStyle::Dot });
            canvas.DrawLine(Point{ x, plotArea.Top() }, Point{ x, plotArea.Bottom() });
            canvas.DrawLine(Point{ plotArea.Left(), y }, Point{ plotArea.Right(), y });

            canvas.SetPen(Pen{ config.palette.tickLabel });
            const auto label = buffer.Fixed(volts, decimals);
            canvas.DrawText(Point{ x - 12.0f, plotArea.Bottom() + 14.0f }, label);
            canvas.DrawText(Point{ plotArea.Left() - 34.0f, y + 4.0f }, label);
        }
    }

    void HexagonCore::DrawAxes(Canvas& canvas, const Rect& plotArea, Point centre, float scale) const
    {
        const auto axisHalfRange = (plotArea.width / 2.0f) / scale;

        canvas.SetFont(FontSpec{ FontFamily::UiDefault, config.axisLabelPointSize });
        DrawTicks(canvas, plotArea, centre, scale, TickStep(axisHalfRange, config.targetTicks));

        canvas.SetPen(Pen{ config.palette.axis });
        canvas.DrawLine(Point{ plotArea.Left(), centre.y }, Point{ plotArea.Right(), centre.y });
        canvas.DrawLine(Point{ centre.x, plotArea.Top() }, Point{ centre.x, plotArea.Bottom() });

        canvas.SetPen(Pen{ config.palette.axisLabel });
        canvas.DrawText(Point{ centre.x + 6.0f, plotArea.Top() - 8.0f }, "\u03b2 (V)");
        canvas.DrawText(Point{ plotArea.Right() + 6.0f, centre.y + 4.0f }, "\u03b1 (V)");
    }

    void HexagonCore::DrawHexagon(Canvas& canvas, Point centre, float scale) const
    {
        const auto radius = twoThirds * dcLinkVolts * scale;

        std::array<Point, corners> hexagon{};
        for (std::size_t k = 0; k != corners; ++k)
        {
            const auto offset = VertexOffset(k, radius);
            hexagon[k] = Point{ centre.x + offset.x, centre.y + offset.y };
        }

        canvas.SetPen(Pen{ config.palette.outline });
        canvas.SetBrush(Brush{ colors::transparent });
        canvas.DrawPolygon(hexagon);

        canvas.SetPen(Pen{ config.palette.sector, 1.0f, LineStyle::Dash });
        for (const auto& corner : hexagon)
            canvas.DrawLine(centre, corner);

        canvas.SetPen(Pen{ config.palette.vertex, 1.0f, LineStyle::None });
        canvas.SetBrush(Brush{ config.palette.vertex });
        for (const auto& corner : hexagon)
            canvas.DrawEllipse(corner, config.vertexRadius, config.vertexRadius);
    }

    void HexagonCore::DrawInscribedCircle(Canvas& canvas, Point centre, float scale) const
    {
        const auto radius = invSqrt3 * dcLinkVolts * scale;

        canvas.SetPen(Pen{ config.palette.inscribedCircle, 2.0f });
        canvas.SetBrush(Brush{ colors::transparent });
        canvas.DrawEllipse(centre, radius, radius);
    }

    void HexagonCore::DrawPhasePhasors(Canvas& canvas, Point centre, float scale) const
    {
        const auto circleRadius = InscribedRadius();
        const std::array<float, 3> magnitudes{ vaSample, vbSample, vcSample };
        const auto peak = std::max({ std::abs(magnitudes[0]), std::abs(magnitudes[1]), std::abs(magnitudes[2]), 1e-6f });

        for (std::size_t i = 0; i != magnitudes.size(); ++i)
        {
            const auto normalised = (magnitudes[i] / peak) * circleRadius;
            const auto tip = ToScreen(centre, normalised * std::cos(phaseAngles[i]), normalised * std::sin(phaseAngles[i]), scale);

            canvas.SetPen(Pen{ config.palette.phase[i], 2.0f });
            canvas.DrawLine(centre, tip);

            canvas.SetPen(Pen{ config.palette.phasorTip, 1.0f, LineStyle::None });
            canvas.SetBrush(Brush{ config.palette.phasorTip });
            canvas.DrawEllipse(tip, config.phasorTipRadius, config.phasorTipRadius);
        }
    }

    void HexagonCore::DrawResultant(Canvas& canvas, Point centre, float scale) const
    {
        const auto magnitude = std::hypot(vAlphaSample, vBetaSample);

        if (magnitude < 1e-9f)
            return;

        const auto scaleFactor = InscribedRadius() / magnitude;
        const auto tip = ToScreen(centre, vAlphaSample * scaleFactor, vBetaSample * scaleFactor, scale);

        canvas.SetPen(Pen{ config.palette.resultant, 3.0f });
        canvas.DrawLine(centre, tip);

        const auto angle = std::atan2(vBetaSample, vAlphaSample);
        const std::array<Point, 3> arrow{
            tip,
            Point{ tip.x - config.arrowHeadLength * std::cos(angle - config.arrowHeadHalfAngle),
                tip.y + config.arrowHeadLength * std::sin(angle - config.arrowHeadHalfAngle) },
            Point{ tip.x - config.arrowHeadLength * std::cos(angle + config.arrowHeadHalfAngle),
                tip.y + config.arrowHeadLength * std::sin(angle + config.arrowHeadHalfAngle) }
        };

        canvas.SetPen(Pen{ config.palette.resultant, 1.0f, LineStyle::None });
        canvas.SetBrush(Brush{ config.palette.resultant });
        canvas.DrawPolygon(arrow);
    }

    void HexagonCore::DrawReadOut(Canvas& canvas, const Rect& plotArea) const
    {
        canvas.SetFont(FontSpec{ FontFamily::Monospace, config.readOutPointSize });

        const auto magnitude = std::hypot(vAlphaSample, vBetaSample);
        const auto sector = SectorOf(vAlphaSample, vBetaSample);

        std::array<std::array<char, 48>, 4> storage{};
        std::array<std::string_view, 4> lines{};

        lines[0] = std::string_view{ storage[0].data(), FormatInto(storage[0], "\u03b1   = {:>7.3f} V", vAlphaSample) };
        lines[1] = std::string_view{ storage[1].data(), FormatInto(storage[1], "\u03b2   = {:>7.3f} V", vBetaSample) };
        lines[2] = std::string_view{ storage[2].data(), FormatInto(storage[2], "|V| = {:>7.3f} V", magnitude) };
        lines[3] = sector == 0
                       ? std::string_view{ storage[3].data(), FormatInto(storage[3], "sector: -") }
                       : std::string_view{ storage[3].data(), FormatInto(storage[3], "sector: {}", sector) };

        auto boxWidth = 0.0f;
        for (const auto& line : lines)
            boxWidth = std::max(boxWidth, canvas.MeasureText(line).width);

        const auto lineHeight = canvas.LineHeight();
        const auto padding = config.readOutPadding;
        const Rect box{ plotArea.Right() - boxWidth - 2.0f * padding, plotArea.Top() + padding,
            boxWidth + 2.0f * padding, lineHeight * static_cast<float>(lines.size()) + 2.0f * padding };

        canvas.SetPen(Pen{ config.palette.readOutFill, 1.0f, LineStyle::None });
        canvas.SetBrush(Brush{ config.palette.readOutFill });
        canvas.DrawRect(box);

        canvas.SetPen(Pen{ config.palette.readOutText });
        for (std::size_t i = 0; i != lines.size(); ++i)
            canvas.DrawText(Point{ box.Left() + padding, box.Top() + padding + static_cast<float>(i + 1) * lineHeight - 4.0f }, lines[i]);
    }
}
