#include "ui/core/PaintedView.hpp"
#include <gmock/gmock.h>

namespace
{
    class MinimalView
        : public ui::PaintedView
    {
    public:
        void Paint(ui::Canvas& canvas, const ui::Rect& bounds) override
        {
            static_cast<void>(canvas);
            static_cast<void>(bounds);
            ++paints;
        }

        void Repaint()
        {
            RequestRepaint();
        }

        int paints{ 0 };
    };

    class WheelOnlyView
        : public MinimalView
    {
    public:
        void OnWheel(const ui::WheelEvent& event) override
        {
            lastDelta = event.delta;
        }

        float lastDelta{ 0.0f };
    };

    class PaintedViewTest
        : public ::testing::Test
    {
    protected:
        MinimalView view;
        WheelOnlyView wheelView;
    };
}

TEST_F(PaintedViewTest, MinimumSizeDefaultsToEmpty)
{
    EXPECT_TRUE(view.MinimumSize().IsEmpty());
}

TEST_F(PaintedViewTest, RepaintWithoutASubscriberIsHarmless)
{
    view.Repaint();

    EXPECT_EQ(view.paints, 0);
}

TEST_F(PaintedViewTest, RepaintNotifiesTheSubscriber)
{
    auto repaints = 0;
    view.onRepaintRequested = [&repaints]
    {
        ++repaints;
    };

    view.Repaint();

    EXPECT_EQ(repaints, 1);
}

// The default handlers exist so a widget overrides only the events it cares about; a view that
// handles the wheel must still tolerate every other event.
TEST_F(PaintedViewTest, UnhandledEventsAreAcceptedSilently)
{
    const ui::MouseEvent mouse{ ui::Point{ 1.0f, 2.0f }, ui::MouseButton::Left, {} };

    wheelView.OnMousePress(mouse);
    wheelView.OnMouseMove(mouse);
    wheelView.OnMouseRelease(mouse);
    wheelView.OnMouseDoubleClick(mouse);
    wheelView.OnMouseLeave();
    wheelView.OnKeyPress(ui::KeyEvent{ ui::Key::Escape, 0, {} });

    EXPECT_NEAR(wheelView.lastDelta, 0.0f, 1e-5f);
}

TEST_F(PaintedViewTest, AnOverriddenHandlerStillReceivesItsEvent)
{
    wheelView.OnWheel(ui::WheelEvent{ ui::Point{ 0.0f, 0.0f }, 1.5f, {} });

    EXPECT_NEAR(wheelView.lastDelta, 1.5f, 1e-5f);
}
