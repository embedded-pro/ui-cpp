#include "ui/backend/recording/RecordingPaintedHost.hpp"
#include <gmock/gmock.h>
#include <memory>

namespace
{
    class CountingView
        : public ui::PaintedView
    {
    public:
        void Paint(ui::Canvas& canvas, const ui::Rect& bounds) override
        {
            static_cast<void>(canvas);
            static_cast<void>(bounds);
        }

        void Repaint()
        {
            RequestRepaint();
        }
    };

    class RecordingPaintedHostTest
        : public ::testing::Test
    {
    protected:
        CountingView view;
    };
}

TEST_F(RecordingPaintedHostTest, ConstructionAttachesToTheView)
{
    const ui::backend::recording::RecordingPaintedHost host{ view };

    EXPECT_TRUE(view.HasHost());
    EXPECT_EQ(host.HostedView(), &view);
    EXPECT_EQ(host.InvalidateCount(), 0u);
    EXPECT_FALSE(host.ViewDestroyed());
}

TEST_F(RecordingPaintedHostTest, EachRepaintRequestIsCounted)
{
    ui::backend::recording::RecordingPaintedHost host{ view };

    view.Repaint();
    view.Repaint();

    EXPECT_EQ(host.InvalidateCount(), 2u);
}

TEST_F(RecordingPaintedHostTest, ClearingTheCountLeavesTheAttachmentIntact)
{
    ui::backend::recording::RecordingPaintedHost host{ view };
    view.Repaint();

    host.ClearInvalidateCount();
    view.Repaint();

    EXPECT_EQ(host.InvalidateCount(), 1u);
}

TEST_F(RecordingPaintedHostTest, AHostDestroyedFirstLeavesTheViewUnhosted)
{
    {
        const ui::backend::recording::RecordingPaintedHost host{ view };
    }

    EXPECT_FALSE(view.HasHost());

    view.Repaint();

    SUCCEED();
}

TEST_F(RecordingPaintedHostTest, AViewDestroyedFirstIsReportedAndClearsThePointer)
{
    auto owned = std::make_unique<CountingView>();
    ui::backend::recording::RecordingPaintedHost host{ *owned };

    owned.reset();

    EXPECT_TRUE(host.ViewDestroyed());
    EXPECT_EQ(host.HostedView(), nullptr);
}
