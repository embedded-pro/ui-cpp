#pragma once

#include "ui/core/Callback.hpp"
#include "ui/core/PaintedView.hpp"
#include "ui/scope/RingBuffer.hpp"
#include "ui/scope/Trigger.hpp"
#include <array>
#include <span>
#include <vector>

namespace ui::scope
{
    // A backend-agnostic oscilloscope: ring buffers, edge triggering and sweep layout, with the
    // trace emitted as one DrawPolyline per channel rather than the per-sample DrawLine the Qt
    // original used.
    class ScopeCore
        : public PaintedView
    {
    public:
        static constexpr std::size_t maxChannels{ 4 };

        explicit ScopeCore(ScopeConfig config = {});

        void SetChannelCount(std::size_t count);
        void SetChannelConfig(std::size_t channel, ChannelConfig config);
        void SetTimePerDivision(float secondsPerDivision);
        void SetSamplePeriod(float seconds);
        void SetTriggerChannel(std::size_t channel);
        void SetTriggerLevel(float level);
        void SetTriggerMode(TriggerMode mode);
        void SetTriggerEdge(TriggerEdge edge);
        void SetRunning(bool running);

        void AddSample(std::span<const float> channelValues);
        void ForceTrigger();
        void Clear();

        void Paint(Canvas& canvas, const Rect& bounds) override;
        [[nodiscard]] Size MinimumSize() const override;

        [[nodiscard]] std::size_t ChannelCount() const;
        [[nodiscard]] float TimePerDivision() const;
        [[nodiscard]] float SamplePeriod() const;
        [[nodiscard]] float TriggerLevel() const;
        [[nodiscard]] TriggerMode CurrentTriggerMode() const;
        [[nodiscard]] TriggerEdge CurrentTriggerEdge() const;
        [[nodiscard]] std::size_t TriggerChannel() const;
        [[nodiscard]] bool IsRunning() const;
        [[nodiscard]] bool IsTriggered() const;
        [[nodiscard]] const RingBuffer& Channel(std::size_t channel) const;

        // Replaces the Qt triggerFired() signal; the backend adapter forwards it if it wants one.
        Callback<void()> onTriggerFired;

    private:
        struct VerticalRange
        {
            float minimum{ 0.0f };
            float maximum{ 0.0f };

            [[nodiscard]] float Span() const;
        };

        [[nodiscard]] Rect PlotAreaFor(const Rect& bounds) const;
        [[nodiscard]] std::size_t SweepSampleCount() const;
        [[nodiscard]] std::size_t FindTriggerPoint() const;

        // The one place the trigger and free-running sweeps differ: everything downstream of the
        // start sample is identical, which is why the Qt original had two near-identical drawing
        // functions.
        [[nodiscard]] std::size_t SweepStartSample() const;

        [[nodiscard]] VerticalRange ComputeVerticalRange(std::size_t startSample, std::size_t sampleCount) const;

        void DrawGrid(Canvas& canvas, const Rect& plotArea) const;
        void DrawTriggerLine(Canvas& canvas, const Rect& plotArea) const;
        void DrawChannels(Canvas& canvas, const Rect& plotArea);
        void DrawChannel(Canvas& canvas, const Rect& plotArea, std::size_t channel, std::size_t startSample, VerticalRange range);
        void DrawLabels(Canvas& canvas, const Rect& plotArea) const;

        ScopeConfig config;

        std::size_t channelCount{ 0 };
        std::array<ChannelConfig, maxChannels> channelConfigs{};
        std::array<RingBuffer, maxChannels> ringBuffers{};

        float timePerDivision{ 0.01f };
        float samplePeriod{ 10e-6f };
        std::size_t triggerChannel{ 0 };
        float triggerLevel{ 0.0f };
        TriggerMode triggerMode{ TriggerMode::Auto };
        TriggerEdge triggerEdge{ TriggerEdge::Rising };

        bool running{ true };
        bool triggered{ false };
        bool singleShotDone{ false };
        float previousTriggerSample{ 0.0f };
        bool hasPreviousTriggerSample{ false };

        std::vector<Point> polylineScratch;
    };
}
