#include "ui/backend/recording/RecordingFormView.hpp"
#include "ui/scope/ScopeController.hpp"
#include <algorithm>
#include <gmock/gmock.h>
#include <optional>

namespace
{
    using ui::backend::recording::FormCommandKind;

    class ScopeControlsTest
        : public ::testing::Test
    {
    protected:
        ScopeControlsTest()
        {
            scope.SetChannelCount(2);
            view.Build(controls.Model());
            controller.emplace(scope, controls.Model(), view);
        }

        [[nodiscard]] const ui::backend::recording::FormCommand* LastActionSpec() const
        {
            const auto& commands = view.Commands();
            const auto match = std::find_if(commands.rbegin(), commands.rend(),
                [](const auto& command)
                {
                    return command.kind == FormCommandKind::SetActionSpec;
                });

            return match == commands.rend() ? nullptr : &*match;
        }

        ui::scope::ScopeCore scope;
        ui::scope::ScopeControls controls{ 2 };
        ui::backend::recording::RecordingFormView view;
        std::optional<ui::scope::ScopeController> controller;
    };
}

TEST_F(ScopeControlsTest, TheFormRealisesEveryScopeControl)
{
    EXPECT_EQ(view.CountOf(FormCommandKind::CreateChoice), 4u);
    EXPECT_EQ(view.CountOf(FormCommandKind::CreateNumber), 1u);
    EXPECT_EQ(view.CountOf(FormCommandKind::CreateAction), 3u);
}

TEST_F(ScopeControlsTest, TheDefaultTimebaseIsTenMillisecondsPerDivision)
{
    EXPECT_NEAR(scope.TimePerDivision(), 10e-3f, 1e-9f);
}

TEST_F(ScopeControlsTest, PickingATimebaseReachesTheScope)
{
    view.PickOption(ui::scope::field::timePerDivision, 0);

    EXPECT_NEAR(scope.TimePerDivision(), 10e-6f, 1e-12f);
}

TEST_F(ScopeControlsTest, PickingATriggerModeReachesTheScope)
{
    view.PickOption(ui::scope::field::triggerMode, 1);

    EXPECT_EQ(scope.CurrentTriggerMode(), ui::scope::TriggerMode::Normal);
}

TEST_F(ScopeControlsTest, PickingATriggerEdgeReachesTheScope)
{
    view.PickOption(ui::scope::field::triggerEdge, 1);

    EXPECT_EQ(scope.CurrentTriggerEdge(), ui::scope::TriggerEdge::Falling);
}

TEST_F(ScopeControlsTest, TypingATriggerLevelReachesTheScope)
{
    view.TypeNumber(ui::scope::field::triggerLevel, -2.5);

    EXPECT_NEAR(scope.TriggerLevel(), -2.5f, 1e-6f);
}

TEST_F(ScopeControlsTest, PickingATriggerChannelReachesTheScope)
{
    view.PickOption(ui::scope::field::triggerChannel, 1);

    EXPECT_EQ(scope.TriggerChannel(), 1u);
}

TEST_F(ScopeControlsTest, AChannelTheScopeDoesNotHaveIsClampedByTheScopeItself)
{
    ui::scope::ScopeCore single;
    single.SetChannelCount(1);
    ui::scope::ScopeControls fourChannels{ 4 };
    ui::backend::recording::RecordingFormView singleView;
    singleView.Build(fourChannels.Model());
    const ui::scope::ScopeController bound{ single, fourChannels.Model(), singleView };

    singleView.PickOption(ui::scope::field::triggerChannel, 3);

    EXPECT_EQ(single.TriggerChannel(), 0u);
}

TEST_F(ScopeControlsTest, TheChannelChoiceOffersOneOptionPerChannel)
{
    ui::scope::ScopeControls four{ 4 };
    ui::backend::recording::RecordingFormView fourView;
    fourView.Build(four.Model());

    const auto& commands = fourView.Commands();
    const auto choice = std::find_if(commands.begin(), commands.end(),
        [](const auto& command)
        {
            return command.kind == FormCommandKind::CreateChoice && command.field == ui::scope::field::triggerChannel;
        });

    ASSERT_NE(choice, commands.end());
    EXPECT_EQ(choice->count, 4u);
}

TEST_F(ScopeControlsTest, AChannelCountBeyondWhatTheScopeHoldsIsClamped)
{
    ui::scope::ScopeControls tooMany{ 99 };
    ui::backend::recording::RecordingFormView clampedView;
    clampedView.Build(tooMany.Model());

    const auto& commands = clampedView.Commands();
    const auto choice = std::find_if(commands.begin(), commands.end(),
        [](const auto& command)
        {
            return command.kind == FormCommandKind::CreateChoice && command.field == ui::scope::field::triggerChannel;
        });

    ASSERT_NE(choice, commands.end());
    EXPECT_EQ(choice->count, ui::scope::ScopeCore::maxChannels);
}

TEST_F(ScopeControlsTest, TheRunStopActionTogglesTheScopeAndItsOwnLabel)
{
    ASSERT_TRUE(scope.IsRunning());

    view.PressAction(ui::scope::field::runStop);

    EXPECT_FALSE(scope.IsRunning());
    ASSERT_NE(LastActionSpec(), nullptr);
    EXPECT_EQ(LastActionSpec()->label, "Run");
    EXPECT_EQ(LastActionSpec()->buttonRole, ui::theme::ButtonRole::Start);
}

TEST_F(ScopeControlsTest, RunningAgainRestoresTheStopLabelAndRole)
{
    view.PressAction(ui::scope::field::runStop);
    view.PressAction(ui::scope::field::runStop);

    EXPECT_TRUE(scope.IsRunning());
    EXPECT_EQ(LastActionSpec()->label, "Stop");
    EXPECT_EQ(LastActionSpec()->buttonRole, ui::theme::ButtonRole::Stop);
}

TEST_F(ScopeControlsTest, SingleArmsASingleShotSweepAndLeavesTheScopeRunning)
{
    view.PressAction(ui::scope::field::runStop);
    ASSERT_FALSE(scope.IsRunning());

    view.PressAction(ui::scope::field::single);

    EXPECT_EQ(scope.CurrentTriggerMode(), ui::scope::TriggerMode::Single);
    EXPECT_TRUE(scope.IsRunning());
}

TEST_F(ScopeControlsTest, SingleAlsoMovesTheTriggerModeChoiceToMatch)
{
    view.PressAction(ui::scope::field::single);

    EXPECT_EQ(controls.Model().Selection(ui::scope::field::triggerMode),
        static_cast<std::size_t>(ui::scope::TriggerMode::Single));
}

TEST_F(ScopeControlsTest, SingleTellsTheViewToRedrawTheTriggerModeChoice)
{
    view.PressAction(ui::scope::field::single);

    const auto& commands = view.Commands();
    const auto refreshed = std::find_if(commands.rbegin(), commands.rend(),
        [](const auto& command)
        {
            return command.kind == FormCommandKind::SetSelection && command.field == ui::scope::field::triggerMode;
        });

    ASSERT_NE(refreshed, commands.rend());
    EXPECT_EQ(refreshed->index, static_cast<std::size_t>(ui::scope::TriggerMode::Single));
}

TEST_F(ScopeControlsTest, ForceTriggersTheSweepWithoutChangingTheMode)
{
    const auto before = scope.CurrentTriggerMode();

    view.PressAction(ui::scope::field::force);

    EXPECT_TRUE(scope.IsTriggered());
    EXPECT_EQ(scope.CurrentTriggerMode(), before);
}

TEST_F(ScopeControlsTest, AControllerThatOutlivesItsModelLeavesNoCallbackBehind)
{
    controller.reset();

    view.PickOption(ui::scope::field::triggerMode, 1);

    EXPECT_EQ(scope.CurrentTriggerMode(), ui::scope::TriggerMode::Auto);
}
