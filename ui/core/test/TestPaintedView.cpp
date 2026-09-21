#include "ui/core/PaintedView.hpp"
#include <gmock/gmock.h>
#include <memory>

namespace
{
    class CountingHost
        : public ui::PaintedViewHost
    {
    public:
        void Invalidate() override
        {
            ++invalidates;
        }

        void OnViewDestroyed() override
        {
            viewDestroyed = true;
        }

        int invalidates{ 0 };
        bool viewDestroyed{ false };
    };

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
        CountingHost host;
    };
}

TEST_F(PaintedViewTest, MinimumSizeDefaultsToEmpty)
{
    EXPECT_TRUE(view.MinimumSize().IsEmpty());
}

TEST_F(PaintedViewTest, RepaintWithoutAHostIsHarmless)
{
    EXPECT_FALSE(view.HasHost());

    view.Repaint();

    EXPECT_EQ(view.paints, 0);
}

TEST_F(PaintedViewTest, RepaintInvalidatesTheAttachedHost)
{
    view.AttachHost(host);

    view.Repaint();

    EXPECT_TRUE(view.HasHost());
    EXPECT_EQ(host.invalidates, 1);
}

TEST_F(PaintedViewTest, ADetachedHostIsNoLongerInvalidated)
{
    view.AttachHost(host);
    view.DetachHost(host);

    view.Repaint();

    EXPECT_FALSE(view.HasHost());
    EXPECT_EQ(host.invalidates, 0);
}

TEST_F(PaintedViewTest, DetachingAHostThatIsNotAttachedChangesNothing)
{
    CountingHost other;
    view.AttachHost(host);

    view.DetachHost(other);

    EXPECT_TRUE(view.HasHost());
}

TEST_F(PaintedViewTest, ADestroyedViewTellsItsHost)
{
    auto destroyed = std::make_unique<MinimalView>();
    destroyed->AttachHost(host);

    destroyed.reset();

    EXPECT_TRUE(host.viewDestroyed);
}

TEST_F(PaintedViewTest, ADestroyedViewThatDetachedFirstTellsNobody)
{
    auto destroyed = std::make_unique<MinimalView>();
    destroyed->AttachHost(host);
    destroyed->DetachHost(host);

    destroyed.reset();

    EXPECT_FALSE(host.viewDestroyed);
}

TEST_F(PaintedViewTest, ARehostedViewInvalidatesOnlyItsNewHost)
{
    CountingHost second;
    view.AttachHost(host);

    view.AttachHost(second);
    view.Repaint();

    EXPECT_EQ(host.invalidates, 0);
    EXPECT_EQ(second.invalidates, 1);
}

TEST_F(PaintedViewTest, AHostDestroyedFirstUnlinksItself)
{
    {
        CountingHost scoped;
        view.AttachHost(scoped);
        EXPECT_TRUE(view.HasHost());
    }

    EXPECT_FALSE(view.HasHost());

    view.Repaint();

    SUCCEED();
}

TEST_F(PaintedViewTest, AttachingOneHostToASecondViewReleasesTheFirst)
{
    MinimalView second;
    view.AttachHost(host);

    second.AttachHost(host);

    EXPECT_FALSE(view.HasHost());
    EXPECT_TRUE(second.HasHost());
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

// wheelView overrides OnWheel, so the base implementation it shadows needs a view that overrides
// nothing at all to be exercised.
TEST_F(PaintedViewTest, TheDefaultWheelHandlerIgnoresItsEvent)
{
    view.OnWheel(ui::WheelEvent{ ui::Point{ 3.0f, 4.0f }, 2.5f, {} });
    view.OnMouseLeave();

    SUCCEED();
}
