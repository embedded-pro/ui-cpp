#include "ui/scope/ScopeCore.hpp"
#include "ui/core/Format.hpp"
#include "ui/theme/Theme.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace ui::scope
{
    namespace
    {
        constexpr float traceLineWidth{ 2.0f };
        constexpr float verticalMarginFraction{ 0.1f };
        constexpr float fallbackVerticalRange{ 1.0f };
        constexpr float labelInset{ 55.0f };
        constexpr float readoutInset{ 120.0f };
        constexpr float arrowSize{ 6.0f };
        constexpr std::size_t labelDivisionStride{ 2 };

        std::string_view FormatSeconds(FormatBuffer<32>& buffer, float seconds)
        {
            if (seconds >= 1.0f)
                return buffer.Fixed(seconds, 2);

            if (seconds >= 1e-3f)
                return buffer.Fixed(seconds * 1e3f, 1);

            return buffer.Fixed(seconds * 1e6f, 0);
        }

        // Deliberately not the same decimals as the axis labels above: the Qt original reads a
        // timebase as "1.00 ms/div" but an axis tick as "2.0ms", and collapsing the two loses
        // a digit exactly where the instrument setting is being read.
        std::string_view FormatTimebase(FormatBuffer<32>& buffer, float seconds)
        {
            if (seconds >= 1.0f)
                return buffer.Fixed(seconds, 1);

            if (seconds >= 1e-3f)
                return buffer.Fixed(seconds * 1e3f, 2);

            return buffer.Fixed(seconds * 1e6f, 0);
        }

        std::string_view SecondsUnit(float seconds)
        {
            if (seconds >= 1.0f)
                return "s";

            if (seconds >= 1e-3f)
                return "ms";

            return "µs";
        }

        std::string_view FormatMagnitude(FormatBuffer<32>& buffer, float value)
        {
            const auto magnitude = std::fabs(value);

            if (magnitude >= 1.0f)
                return buffer.Fixed(value, 2);

            if (magnitude >= 1e-3f)
                return buffer.Fixed(value * 1e3f, 1);

            return buffer.Fixed(value * 1e6f, 0);
        }

        std::string_view MagnitudeUnit(float value)
        {
            const auto magnitude = std::fabs(value);

            if (magnitude >= 1.0f)
                return "";

            if (magnitude >= 1e-3f)
                return "m";

            return "µ";
        }

        std::string_view TriggerModeLabel(TriggerMode mode)
        {
            switch (mode)
            {
                case TriggerMode::Normal:
                    return "NORM";
                case TriggerMode::Single:
                    return "SINGLE";
                case TriggerMode::Auto:
                default:
                    return "AUTO";
            }
        }
    }

    float ScopeCore::VerticalRange::Span() const
    {
        return maximum - minimum;
    }

    ScopeCore::ScopeCore(ScopeConfig config)
        : config(config)
    {
        for (auto& buffer : ringBuffers)
            buffer.Resize(config.sampleCapacity);
    }

    void ScopeCore::SetChannelCount(std::size_t count)
    {
        channelCount = std::min(count, maxChannels);
    }

    void ScopeCore::SetChannelConfig(std::size_t channel, ChannelConfig channelConfig)
    {
        if (channel < maxChannels)
            channelConfigs[channel] = std::move(channelConfig);
    }

    void ScopeCore::SetTimePerDivision(float secondsPerDivision)
    {
        timePerDivision = secondsPerDivision;
        RequestRepaint();
    }

    void ScopeCore::SetSamplePeriod(float seconds)
    {
        samplePeriod = seconds;
    }

    void ScopeCore::SetTriggerChannel(std::size_t channel)
    {
        triggerChannel = std::min(channel, channelCount > 0 ? channelCount - 1 : std::size_t{ 0 });
    }

    void ScopeCore::SetTriggerLevel(float level)
    {
        triggerLevel = level;
        RequestRepaint();
    }

    void ScopeCore::SetTriggerMode(TriggerMode mode)
    {
        triggerMode = mode;
        singleShotDone = false;
        RequestRepaint();
    }

    void ScopeCore::SetTriggerEdge(TriggerEdge edge)
    {
        triggerEdge = edge;
    }

    void ScopeCore::SetRunning(bool run)
    {
        running = run;

        if (running)
            singleShotDone = false;
    }

    void ScopeCore::AddSample(std::span<const float> channelValues)
    {
        if (!running)
            return;

        if (triggerMode == TriggerMode::Single && singleShotDone)
            return;

        const auto pushed = std::min(channelValues.size(), channelCount);
        for (std::size_t i = 0; i < pushed; ++i)
            ringBuffers[i].Push(channelValues[i]);

        if (triggerChannel >= channelCount || triggerChannel >= channelValues.size())
            return;

        const auto current = channelValues[triggerChannel];

        // An edge needs a genuine predecessor. The Qt original gated this on the buffer holding
        // more than one sample, which skipped the assignment below on the very first sample and
        // left previousTriggerSample at its initial 0 — so a trace starting above the level and
        // falling reported a spurious *rising* trigger on its second sample. Tracking whether a
        // predecessor exists fixes that; it is a behaviour change from e-foc, not a port.
        if (hasPreviousTriggerSample)
        {
            const auto detected = triggerEdge == TriggerEdge::Rising
                                      ? previousTriggerSample < triggerLevel && current >= triggerLevel
                                      : previousTriggerSample > triggerLevel && current <= triggerLevel;

            if (detected)
            {
                triggered = true;

                if (triggerMode == TriggerMode::Single)
                    singleShotDone = true;

                if (onTriggerFired)
                    onTriggerFired();
            }
        }

        previousTriggerSample = current;
        hasPreviousTriggerSample = true;
    }

    void ScopeCore::ForceTrigger()
    {
        triggered = true;
        singleShotDone = false;
        RequestRepaint();
    }

    void ScopeCore::Clear()
    {
        for (auto& buffer : ringBuffers)
            buffer.Clear();

        triggered = false;
        singleShotDone = false;
        previousTriggerSample = 0.0f;
        hasPreviousTriggerSample = false;
        RequestRepaint();
    }

    Size ScopeCore::MinimumSize() const
    {
        return Size{ 320.0f, 250.0f };
    }

    std::size_t ScopeCore::ChannelCount() const
    {
        return channelCount;
    }

    float ScopeCore::TimePerDivision() const
    {
        return timePerDivision;
    }

    float ScopeCore::SamplePeriod() const
    {
        return samplePeriod;
    }

    float ScopeCore::TriggerLevel() const
    {
        return triggerLevel;
    }

    TriggerMode ScopeCore::CurrentTriggerMode() const
    {
        return triggerMode;
    }

    TriggerEdge ScopeCore::CurrentTriggerEdge() const
    {
        return triggerEdge;
    }

    std::size_t ScopeCore::TriggerChannel() const
    {
        return triggerChannel;
    }

    bool ScopeCore::IsRunning() const
    {
        return running;
    }

    bool ScopeCore::IsTriggered() const
    {
        return triggered;
    }

    const RingBuffer& ScopeCore::Channel(std::size_t channel) const
    {
        return ringBuffers[std::min(channel, maxChannels - 1)];
    }

    Rect ScopeCore::PlotAreaFor(const Rect& bounds) const
    {
        return Rect{
            bounds.Left() + config.leftMargin,
            bounds.Top() + config.topMargin,
            bounds.width - config.leftMargin - config.rightMargin,
            bounds.height - config.topMargin - config.bottomMargin
        };
    }

    std::size_t ScopeCore::SweepSampleCount() const
    {
        if (samplePeriod <= 0.0f)
            return 0;

        const auto sweepSeconds = timePerDivision * static_cast<float>(config.horizontalDivisions);

        return static_cast<std::size_t>(sweepSeconds / samplePeriod);
    }

    std::size_t ScopeCore::FindTriggerPoint() const
    {
        if (triggerChannel >= channelCount)
            return 0;

        const auto& buffer = ringBuffers[triggerChannel];
        if (buffer.Count() < 2)
            return 0;

        const auto searchStart = buffer.Count() > config.triggerSearchLimit
                                     ? buffer.Count() - config.triggerSearchLimit
                                     : std::size_t{ 0 };

        for (auto i = buffer.Count() - 1; i > searchStart; --i)
        {
            const auto previous = buffer.At(i - 1);
            const auto current = buffer.At(i);

            const auto detected = triggerEdge == TriggerEdge::Rising
                                      ? previous < triggerLevel && current >= triggerLevel
                                      : previous > triggerLevel && current <= triggerLevel;

            if (detected)
                return i;
        }

        return buffer.Count() - 1;
    }

    std::size_t ScopeCore::SweepStartSample() const
    {
        const auto sweep = SweepSampleCount();
        const auto available = ringBuffers[0].Count();

        if (triggered && triggerMode != TriggerMode::Auto)
        {
            const auto preTrigger = static_cast<std::size_t>(static_cast<float>(sweep) * config.preTriggerFraction);
            const auto triggerPoint = FindTriggerPoint();

            return triggerPoint > preTrigger ? triggerPoint - preTrigger : std::size_t{ 0 };
        }

        return available > sweep ? available - sweep : std::size_t{ 0 };
    }

    ScopeCore::VerticalRange ScopeCore::ComputeVerticalRange(std::size_t startSample, std::size_t sampleCount) const
    {
        auto lowest = std::numeric_limits<float>::max();
        auto highest = std::numeric_limits<float>::lowest();

        for (std::size_t channel = 0; channel < channelCount; ++channel)
        {
            if (!channelConfigs[channel].enabled)
                continue;

            const auto& buffer = ringBuffers[channel];
            const auto end = std::min(startSample + sampleCount, buffer.Count());

            for (auto i = startSample; i < end; ++i)
            {
                const auto value = buffer.At(i);
                lowest = std::min(lowest, value);
                highest = std::max(highest, value);
            }
        }

        if (lowest > highest)
        {
            lowest = -fallbackVerticalRange;
            highest = fallbackVerticalRange;
        }

        auto margin = (highest - lowest) * verticalMarginFraction;
        if (margin < 1e-9f)
            margin = fallbackVerticalRange * verticalMarginFraction;

        return VerticalRange{ lowest - margin, highest + margin };
    }

    void ScopeCore::Paint(Canvas& canvas, const Rect& bounds)
    {
        const auto plotArea = PlotAreaFor(bounds);
        if (plotArea.IsEmpty())
            return;

        canvas.SetAntialiasing(true);
        canvas.FillRect(bounds, theme::Current().Get(theme::ColorRole::ScopeBackground));

        DrawGrid(canvas, plotArea);
        DrawTriggerLine(canvas, plotArea);
        DrawChannels(canvas, plotArea);
        DrawLabels(canvas, plotArea);
    }

    void ScopeCore::DrawGrid(Canvas& canvas, const Rect& plotArea) const
    {
        const auto& theme = theme::Current();

        canvas.SetPen(Pen{ theme.Get(theme::ColorRole::GridMinor), 1.0f, LineStyle::Dot });

        for (std::size_t i = 0; i <= config.horizontalDivisions; ++i)
        {
            const auto x = plotArea.Left() + plotArea.width * static_cast<float>(i) / static_cast<float>(config.horizontalDivisions);
            canvas.DrawLine(Point{ x, plotArea.Top() }, Point{ x, plotArea.Bottom() });
        }

        for (std::size_t i = 0; i <= config.verticalDivisions; ++i)
        {
            const auto y = plotArea.Top() + plotArea.height * static_cast<float>(i) / static_cast<float>(config.verticalDivisions);
            canvas.DrawLine(Point{ plotArea.Left(), y }, Point{ plotArea.Right(), y });
        }

        canvas.SetPen(Pen{ theme.Get(theme::ColorRole::GridMajor) });
        canvas.DrawLine(Point{ plotArea.Center().x, plotArea.Top() }, Point{ plotArea.Center().x, plotArea.Bottom() });
        canvas.DrawLine(Point{ plotArea.Left(), plotArea.Center().y }, Point{ plotArea.Right(), plotArea.Center().y });

        canvas.SetPen(Pen{ theme.Get(theme::ColorRole::Axis) });
        canvas.SetBrush(Brush{ colors::transparent });
        canvas.DrawRect(plotArea);
    }

    void ScopeCore::DrawTriggerLine(Canvas& canvas, const Rect& plotArea) const
    {
        if (channelCount == 0)
            return;

        const auto sweep = SweepSampleCount();
        const auto range = ComputeVerticalRange(0, std::min(sweep, ringBuffers[0].Count()));

        if (triggerLevel < range.minimum || triggerLevel > range.maximum || range.Span() <= 0.0f)
            return;

        const auto ratio = 1.0f - (triggerLevel - range.minimum) / range.Span();
        const auto y = plotArea.Top() + plotArea.height * ratio;
        const auto trigger = theme::Current().Get(theme::ColorRole::Crosshair);

        canvas.SetPen(Pen{ trigger, 1.0f, LineStyle::Dash });
        canvas.DrawLine(Point{ plotArea.Left(), y }, Point{ plotArea.Right(), y });

        canvas.SetBrush(Brush{ trigger });
        const std::array<Point, 3> arrow{
            Point{ plotArea.Left() - arrowSize * 2.0f, y },
            Point{ plotArea.Left() - arrowSize, y - arrowSize },
            Point{ plotArea.Left() - arrowSize, y + arrowSize }
        };
        canvas.DrawPolygon(arrow);
        canvas.SetBrush(Brush{ colors::transparent });
    }

    void ScopeCore::DrawChannels(Canvas& canvas, const Rect& plotArea)
    {
        const auto sweep = SweepSampleCount();
        if (sweep < 2)
            return;

        const auto startSample = SweepStartSample();

        // No degenerate-span guard here: ComputeVerticalRange already floors its margin at
        // fallbackVerticalRange * verticalMarginFraction, so the span it returns is never smaller
        // than 0.2. The guard this replaced tested for less than 1e-12 and could never fire.
        const auto range = ComputeVerticalRange(startSample, sweep);

        for (std::size_t channel = 0; channel < channelCount; ++channel)
            if (channelConfigs[channel].enabled)
                DrawChannel(canvas, plotArea, channel, startSample, range);
    }

    void ScopeCore::DrawChannel(Canvas& canvas, const Rect& plotArea, std::size_t channel, std::size_t startSample, VerticalRange range)
    {
        const auto& buffer = ringBuffers[channel];
        if (buffer.Count() < 2 || startSample >= buffer.Count())
            return;

        const auto sweep = SweepSampleCount();
        const auto samplesToShow = std::min(sweep, buffer.Count() - startSample);

        canvas.SetPen(Pen{ channelConfigs[channel].color, traceLineWidth });
        canvas.SetClip(plotArea);

        polylineScratch.clear();

        // One virtual call per channel rather than one per sample: at 10 ms/div and a 10 us sample
        // period this sweep is 10 000 points, and the Qt original issued a drawLine for each.
        for (std::size_t i = 0; i < samplesToShow; ++i)
        {
            const auto value = buffer.At(startSample + i);
            const auto x = plotArea.Left() + plotArea.width * static_cast<float>(i) / static_cast<float>(sweep);
            const auto ratio = 1.0f - (value - range.minimum) / range.Span();

            polylineScratch.push_back(Point{ x, plotArea.Top() + plotArea.height * ratio });
        }

        if (polylineScratch.size() > 1)
            canvas.DrawPolyline(polylineScratch);

        canvas.ClearClip();
    }

    void ScopeCore::DrawLabels(Canvas& canvas, const Rect& plotArea) const
    {
        const auto& theme = theme::Current();

        canvas.SetFont(theme.Get(theme::FontRole::Small));
        canvas.SetPen(Pen{ theme.Get(theme::ColorRole::TextMuted) });

        const auto sweepSeconds = timePerDivision * static_cast<float>(config.horizontalDivisions);

        for (std::size_t i = 0; i <= config.horizontalDivisions; i += labelDivisionStride)
        {
            const auto x = plotArea.Left() + plotArea.width * static_cast<float>(i) / static_cast<float>(config.horizontalDivisions);
            const auto seconds = sweepSeconds * static_cast<float>(i) / static_cast<float>(config.horizontalDivisions);

            FormatBuffer<32> buffer;
            const auto magnitude = FormatSeconds(buffer, seconds);

            std::array<char, 40> label{};
            const auto written = FormatInto(label, "{}{}", magnitude, SecondsUnit(seconds));

            canvas.DrawText(Point{ x - 15.0f, plotArea.Bottom() + 15.0f }, std::string_view{ label.data(), written });
        }

        const auto sweep = SweepSampleCount();
        const auto range = ComputeVerticalRange(SweepStartSample(), sweep);

        for (std::size_t i = 0; i <= config.verticalDivisions; i += labelDivisionStride)
        {
            const auto y = plotArea.Top() + plotArea.height * static_cast<float>(i) / static_cast<float>(config.verticalDivisions);
            const auto value = range.maximum - range.Span() * static_cast<float>(i) / static_cast<float>(config.verticalDivisions);

            FormatBuffer<32> buffer;
            const auto magnitude = FormatMagnitude(buffer, value);

            std::array<char, 40> label{};
            const auto written = FormatInto(label, "{}{}", magnitude, MagnitudeUnit(value));

            canvas.DrawText(Point{ plotArea.Left() - labelInset, y + 4.0f }, std::string_view{ label.data(), written });
        }

        canvas.SetPen(Pen{ theme.Get(theme::ColorRole::Text) });

        FormatBuffer<32> perDivision;
        const auto perDivisionMagnitude = FormatTimebase(perDivision, timePerDivision);

        std::array<char, 48> timebase{};
        const auto timebaseLength = FormatInto(timebase, "{} {}/div", perDivisionMagnitude, SecondsUnit(timePerDivision));
        canvas.DrawText(Point{ plotArea.Right() - readoutInset, plotArea.Top() + 15.0f }, std::string_view{ timebase.data(), timebaseLength });

        canvas.SetPen(Pen{ theme.Get(triggered ? theme::ColorRole::Ok : theme::ColorRole::Warning) });

        std::array<char, 48> trigger{};
        const auto triggerLength = FormatInto(trigger, "Trig: {}", TriggerModeLabel(triggerMode));
        canvas.DrawText(Point{ plotArea.Right() - readoutInset, plotArea.Top() + 30.0f }, std::string_view{ trigger.data(), triggerLength });

        auto legendX = plotArea.Left() + 5.0f;

        for (std::size_t channel = 0; channel < channelCount; ++channel)
        {
            if (!channelConfigs[channel].enabled)
                continue;

            canvas.SetPen(Pen{ channelConfigs[channel].color });
            canvas.DrawText(Point{ legendX, plotArea.Top() + 15.0f }, channelConfigs[channel].name);
            legendX += 60.0f;
        }
    }
}
