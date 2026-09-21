#include "ui/scope/ScopeCore.hpp"
#include "ui/theme/Theme.hpp"
#include <gmock/gmock.h>

namespace
{
    using ui::scope::TriggerEdge;
    using ui::scope::TriggerMode;

    ui::scope::ScopeConfig SmallScope()
    {
        ui::scope::ScopeConfig config;
        config.sampleCapacity = 1024;

        return config;
    }

    class TriggerTest
        : public ::testing::Test
    {
    protected:
        TriggerTest()
        {
            scope.SetChannelCount(1);
            scope.SetChannelConfig(0, ui::scope::ChannelConfig{ "ia", ui::theme::Light().Series(0), true });
            scope.SetTriggerChannel(0);
            scope.SetTriggerLevel(0.5f);
        }

        void Feed(std::initializer_list<float> values)
        {
            for (auto value : values)
            {
                const std::array<float, 1> sample{ value };
                scope.AddSample(sample);
            }
        }

        std::size_t fired{ 0 };
        ui::scope::ScopeCore scope{ SmallScope() };
    };
}

TEST_F(TriggerTest, ARisingEdgeThroughTheLevelFires)
{
    scope.SetTriggerEdge(TriggerEdge::Rising);
    Feed({ 0.0f, 0.2f, 0.4f, 0.9f });

    EXPECT_TRUE(scope.IsTriggered());
}

TEST_F(TriggerTest, AFallingEdgeDoesNotFireARisingTrigger)
{
    scope.SetTriggerEdge(TriggerEdge::Rising);
    Feed({ 1.0f, 0.9f, 0.4f, 0.0f });

    EXPECT_FALSE(scope.IsTriggered());
}

TEST_F(TriggerTest, AFallingEdgeThroughTheLevelFires)
{
    scope.SetTriggerEdge(TriggerEdge::Falling);
    Feed({ 1.0f, 0.9f, 0.4f, 0.0f });

    EXPECT_TRUE(scope.IsTriggered());
}

TEST_F(TriggerTest, ARisingEdgeDoesNotFireAFallingTrigger)
{
    scope.SetTriggerEdge(TriggerEdge::Falling);
    Feed({ 0.0f, 0.2f, 0.4f, 0.9f });

    EXPECT_FALSE(scope.IsTriggered());
}

TEST_F(TriggerTest, ASignalStayingBelowTheLevelNeverFires)
{
    scope.SetTriggerEdge(TriggerEdge::Rising);
    Feed({ 0.0f, 0.1f, 0.2f, 0.3f, 0.4f });

    EXPECT_FALSE(scope.IsTriggered());
}

TEST_F(TriggerTest, TouchingTheLevelExactlyCountsAsCrossingIt)
{
    scope.SetTriggerEdge(TriggerEdge::Rising);
    Feed({ 0.0f, 0.5f });

    EXPECT_TRUE(scope.IsTriggered());
}

TEST_F(TriggerTest, TheCallbackFiresOnceForOneCrossing)
{
    scope.onTriggerFired = [this]
    {
        ++fired;
    };

    scope.SetTriggerEdge(TriggerEdge::Rising);
    Feed({ 0.0f, 0.2f, 0.9f, 1.0f, 1.2f });

    EXPECT_EQ(fired, 1u);
}

TEST_F(TriggerTest, EachSeparateCrossingFiresAgain)
{
    scope.onTriggerFired = [this]
    {
        ++fired;
    };

    scope.SetTriggerEdge(TriggerEdge::Rising);
    Feed({ 0.0f, 0.9f, 0.0f, 0.9f, 0.0f, 0.9f });

    EXPECT_EQ(fired, 3u);
}

TEST_F(TriggerTest, SingleModeStopsAcquiringAfterTheFirstCrossing)
{
    scope.SetTriggerMode(TriggerMode::Single);
    scope.SetTriggerEdge(TriggerEdge::Rising);
    Feed({ 0.0f, 0.9f });

    const auto countAtTrigger = scope.Channel(0).Count();
    Feed({ 0.0f, 0.9f, 0.0f });

    EXPECT_EQ(scope.Channel(0).Count(), countAtTrigger);
}

TEST_F(TriggerTest, NormalModeKeepsAcquiringAfterACrossing)
{
    scope.SetTriggerMode(TriggerMode::Normal);
    scope.SetTriggerEdge(TriggerEdge::Rising);
    Feed({ 0.0f, 0.9f });

    const auto countAtTrigger = scope.Channel(0).Count();
    Feed({ 0.0f, 0.9f, 0.0f });

    EXPECT_GT(scope.Channel(0).Count(), countAtTrigger);
}

TEST_F(TriggerTest, ReArmingASingleShotResumesAcquisition)
{
    scope.SetTriggerMode(TriggerMode::Single);
    scope.SetTriggerEdge(TriggerEdge::Rising);
    Feed({ 0.0f, 0.9f });

    const auto countAtTrigger = scope.Channel(0).Count();

    scope.SetRunning(true);
    Feed({ 0.0f, 0.9f });

    EXPECT_GT(scope.Channel(0).Count(), countAtTrigger);
}

TEST_F(TriggerTest, AStoppedScopeAcquiresNothing)
{
    scope.SetRunning(false);
    Feed({ 0.0f, 0.9f, 1.0f });

    EXPECT_EQ(scope.Channel(0).Count(), 0u);
    EXPECT_FALSE(scope.IsTriggered());
}

TEST_F(TriggerTest, ForcingATriggerFiresWithoutACrossing)
{
    Feed({ 0.0f, 0.1f });
    EXPECT_FALSE(scope.IsTriggered());

    scope.ForceTrigger();

    EXPECT_TRUE(scope.IsTriggered());
}

TEST_F(TriggerTest, ClearingDisarmsTheTrigger)
{
    scope.SetTriggerEdge(TriggerEdge::Rising);
    Feed({ 0.0f, 0.9f });
    ASSERT_TRUE(scope.IsTriggered());

    scope.Clear();

    EXPECT_FALSE(scope.IsTriggered());
    EXPECT_EQ(scope.Channel(0).Count(), 0u);
}

TEST_F(TriggerTest, ATriggerChannelBeyondTheChannelCountIsClamped)
{
    scope.SetTriggerChannel(3);

    EXPECT_EQ(scope.TriggerChannel(), 0u);
}

TEST_F(TriggerTest, SamplesBeyondTheChannelCountAreIgnored)
{
    const std::array<float, 4> wide{ 0.1f, 0.2f, 0.3f, 0.4f };
    scope.AddSample(wide);

    EXPECT_EQ(scope.Channel(0).Count(), 1u);
    EXPECT_EQ(scope.Channel(1).Count(), 0u);
}
