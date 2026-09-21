#pragma once

#include "ui/core/Color.hpp"
#include "ui/core/PaintedView.hpp"
#include <array>
#include <cstddef>

namespace ui::widgets
{
    struct HexagonConfig
    {
        float plotMargin{ 40.0f };
        std::size_t targetTicks{ 6 };

        std::uint8_t axisLabelPointSize{ 9 };
        std::uint8_t readOutPointSize{ 10 };

        float vertexRadius{ 5.0f };
        float phasorTipRadius{ 4.0f };
        float arrowHeadLength{ 14.0f };
        float arrowHeadHalfAngle{ 0.45f };
        float readOutPadding{ 6.0f };

        float attack{ 0.30f };
        float decay{ 0.0015f };

        Color grid{ 50, 50, 50 };
        Color tickLabel{ 170, 170, 170 };
        Color axis{ 180, 180, 180 };
        Color axisLabel{ 220, 220, 220 };
        Color outline{ 160, 160, 160 };
        Color sector{ 90, 90, 90 };
        Color vertex{ 240, 200, 60 };
        Color inscribedCircle{ 60, 150, 230 };
        std::array<Color, 3> phase{ Color{ 230, 90, 60 }, Color{ 120, 200, 90 }, Color{ 230, 160, 60 } };
        Color phasorTip{ 180, 130, 200 };
        Color resultant{ 255, 220, 60 };
        Color readOutFill{ 20, 20, 20, 200 };
        Color readOutText{ 220, 220, 220 };
    };

    class HexagonCore
        : public PaintedView
    {
    public:
        explicit HexagonCore(HexagonConfig config = {});

        void SetDcLinkVolts(float volts);
        void SetSample(float va, float vb, float vc, float vAlpha, float vBeta);
        void Clear();

        void Paint(Canvas& canvas, const Rect& bounds) override;
        [[nodiscard]] Size MinimumSize() const override;

        [[nodiscard]] static int SectorOf(float vAlpha, float vBeta);
        [[nodiscard]] static float TickStep(float axisHalfRange, std::size_t targetTicks);
        [[nodiscard]] static Point VertexOffset(std::size_t corner, float radius);

        [[nodiscard]] float PeakMagnitude() const;
        [[nodiscard]] float DcLinkVolts() const;

    private:
        [[nodiscard]] float AxisHalfRange() const;
        [[nodiscard]] float InscribedRadius() const;

        void DrawAxes(Canvas& canvas, const Rect& plotArea, Point centre, float scale) const;
        void DrawHexagon(Canvas& canvas, Point centre, float scale) const;
        void DrawInscribedCircle(Canvas& canvas, Point centre, float scale) const;
        void DrawPhasePhasors(Canvas& canvas, Point centre, float scale) const;
        void DrawResultant(Canvas& canvas, Point centre, float scale) const;
        void DrawReadOut(Canvas& canvas, const Rect& plotArea) const;

        HexagonConfig config;

        float dcLinkVolts{ 24.0f };
        float vaSample{ 0.0f };
        float vbSample{ 0.0f };
        float vcSample{ 0.0f };
        float vAlphaSample{ 0.0f };
        float vBetaSample{ 0.0f };
        float peakMagnitude{ 0.0f };
    };
}
